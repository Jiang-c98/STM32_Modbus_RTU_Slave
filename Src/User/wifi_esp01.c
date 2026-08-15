/**
 * @file    wifi_esp01.c
 * @brief   WIFI模块
 * @author  Cui Jiang
 * @date    2025-08-11
 */

/* 头文件包含 */
#include "User/wifi_esp01.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "User/share_data.h"
#include "User/sensor.h"
#include "User/bsp_usart.h"
#include "usart.h"

/* 私有宏定义 */
#define WIFI_WATCHDOG_CHECK_INTERVAL 10000 //10秒检查一次
#define WIFI_WATCHDOG_MAX_FAIL_COUNT 3    //连续失败3次触发复位

/* 私有变量 */
static ring_buffer_t wifi_rx_ring;        //缓冲区变量
static WifiState_t wifi_state = WIFI_IDLE;//状态机
static uint8_t wifi_init_flag = 0;        //wifi模块初始化标志
volatile static uint8_t wifi_line_ready;  //完整行标志
static char wifi_line_buffer[512];        //行缓冲区
static uint16_t wifi_line_len;            //行数据长度
static char tcp_pending_data[64];         //延迟发送TCP数据缓冲区
static uint8_t wifi_cmd_success;          //wifi指令发送成功标志
static uint32_t wifi_state_start_time;    //状态机开始时间
static uint8_t wifi_rx_pop(uint8_t *data);

/* 全局变量 */
uint8_t Wifi_byte;

/* 私有函数声明 */
static void handle_tcp_command(char *data, uint8_t link_id);
static void wifi_send_via_tcp(char *data, uint16_t len, uint8_t link_id);
static void wifi_force_reset(void);
static uint8_t wifi_send_cmd_with_response(const char *cmd, uint32_t timeout_ms);
static uint8_t Wifi_AT_Init(void);
static void wifi_process(void);
static void wifi_state_machine(char *line);


/**
 * @brief WIFI看门狗任务
 * @note 10秒检查一次模块状态，状态机是否卡死及模块是否活跃
 */
static void vWifi_Watchdog_Task(void *pvParameters){
  uint8_t fail_count = 0;
	for(;;){
		//5秒检测一次
	  vTaskDelay(pdMS_TO_TICKS(WIFI_WATCHDOG_CHECK_INTERVAL));
		if(wifi_init_flag == 0)continue;
		
		//1 检查ESP-01S模块是否正常,发AT指令测试
		if(wifi_send_cmd_with_response("AT\r\n", 300) != 1){
		  fail_count++;
		  printf("[WDOG] No response, fail_count = %d\r\n", fail_count);
		}


		if(fail_count >= WIFI_WATCHDOG_MAX_FAIL_COUNT){
			printf("[WDOG] Too many fails, reset wifi\r\n");
			wifi_force_reset();
			fail_count = 0;	
		}
	}
}


/**
 * @brief WIFI任务
 * @note 首次上电等待3秒，开始初始化
 *       进入循环检查缓冲区完整行，解析
 *       超时5秒重置状态机
 */
static void vWifi_Task(void *Parameters){
	//等待ESP-01S上电稳定工作
	vTaskDelay(pdMS_TO_TICKS(3000));
	
	//执行初始化
	Wifi_AT_Init();
	
  for(;;){
		//1. 从缓存区取数据，拼成行
		wifi_process();

		//2. 有完整一行，交给状态机处理
		if(wifi_line_ready){
			wifi_line_ready = 0;
			wifi_state_machine(wifi_line_buffer);
		}
		
		//3. 状态机超时保护
		if(wifi_state != WIFI_IDLE){
			if(HAL_GetTick() - wifi_state_start_time > 5000){
				wifi_state = WIFI_IDLE;  // 超时复位，防止卡死
				wifi_cmd_success = 0;
				printf("[WIFI] State timeout, reset to IDLE\r\n");
			}
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

/**
 * @brief WIFI任务
 * @note 启动WIFI功能
 */
void StartWifiTask(void){
	//创建WIFI主任务
	if(xTaskCreate(vWifi_Task, "wifi", 512, NULL, 2, NULL) != pdPASS){
	  printf("vWifi_Task create failed!\r\n");
	}

	//创建WIFI看门狗任务（优先级最低）
	if(xTaskCreate(vWifi_Watchdog_Task, "wifi_watchdog", 128, NULL, 1, NULL) != pdPASS){
	  printf("vWifi_Watchdog_Task create failed!\r\n");
	}
}

/**
 * @brief AT指令发送函数
 * @note 发送AT指令,不等回复
 */
static uint8_t wifi_send_raw(const char *cmd){
  uint16_t len = strlen(cmd);
	if(HAL_UART_Transmit(&huart2, (uint8_t *)cmd, len, 100) != HAL_OK){
	  printf("Send Raw \"%s\" failed", cmd);
	}
	return 1;
}

/**
 * @brief 发送指令函数，带超时保护
 * @note 区分初始化阶段和正常运行阶段驱动状态机方式
 */
static uint8_t wifi_send_cmd_with_response(const char *cmd, uint32_t timeout_ms){
  if(wifi_state != WIFI_IDLE){
	  printf("[WIFI] Busy, can't send\r\n");
		return 0;
	}
	wifi_send_raw(cmd);
	wifi_state = WIFI_WAITING_OK;
	wifi_state_start_time = HAL_GetTick();
	wifi_cmd_success = 0;
	
  //等待状态机回到IDLE，阻塞等待，但可以被超时打断
		while(wifi_state != WIFI_IDLE){
			//初始化阶段，主动驱动状态机
			if(wifi_init_flag == 0){
			  wifi_process();
				if(wifi_line_ready){
					wifi_line_ready = 0;
					wifi_state_machine(wifi_line_buffer);
				}
		  }
			
			//正常运行阶段，不主动驱动状态机，由vWifi_Task循环驱动
			if(HAL_GetTick() - wifi_state_start_time > timeout_ms){
				wifi_state = WIFI_IDLE;
				printf("[WIFI] Wait OK timeout\r\n");
				return 0;
			}
			vTaskDelay(pdMS_TO_TICKS(10));
		}
	return wifi_cmd_success;
}

/**
 * @brief WiFi初始化
 * @note 打开串口中断接收数据
 */
void Wifi_Init(void){
	HAL_UART_Receive_IT(&huart2, &Wifi_byte, 1);
}

/**
 * @brief WiFi_AT指令初始化
 * @note 配置ESP-01S的状态
 */
static uint8_t Wifi_AT_Init(void){
  if(wifi_send_cmd_with_response("AT\r\n", 500) != 1){
	  printf("[ERR] Module not responding!\r\n");
	}
	// 关掉回显
	wifi_send_cmd_with_response("ATE0\r\n", 300);
	//设置为 AP+STA 模式
	wifi_send_cmd_with_response("AT+CWMODE=3\r\n", 300);
	//开启多连接
	wifi_send_cmd_with_response("AT+CIPMUX=1\r\n", 300);
	//创建 TCP Server，端口 8080
	wifi_send_cmd_with_response("AT+CIPSERVER=1,8080\r\n", 300);
	
	//可选：查询 IP
	wifi_send_cmd_with_response("AT+CIFSR\r\n", 300);
	
	printf("[WiFi] Server started, waiting for client...\r\n");
	wifi_init_flag = 1;
	return 1;
}


/**
 * @brief 环形缓冲区取数据，拼成一行
 * @note 判断是否有数据，末尾字符是否为'\n'换行，拼接'\0'结束符
 */
static void wifi_process(void){
	uint8_t c;
	while(wifi_rx_pop(&c)){//
    if(c == '\n'){
			if(wifi_line_len > 0){
				wifi_line_buffer[wifi_line_len] = '\0';
				wifi_line_ready = 1;
				wifi_line_len = 0;
			}
		}else if(c != '\r' && wifi_line_len < sizeof(wifi_line_buffer) -1){
      wifi_line_buffer[wifi_line_len++] = c;
		}
	}
}

/**
 * @brief 应用层处理,解析TCP数据
 * @param data数组指针
 * @param 连接ID
 * @note 收到的数据做对应的动作，并将成功的结果返回给TCP客户端
 */
void handle_tcp_command(char *data, uint8_t link_id){
	if(data == NULL || data[0] == '\0' || data[0] == '\r' || data[0] == '\n'){
	  printf("[TCP] Empty data, ignore\r\n");
		wifi_state = WIFI_IDLE;  // 强制复位状态机
		return;
	}
	//数据采集指令
	printf("[TCP] Received: '%s'\r\n", data);
	if(strstr(data, "LED_OFF") != NULL){
		//LED_OFF_OK
		ShareData_SetReg(4, 0);
		wifi_send_via_tcp("LED_OFF_OK\r\n", 12, link_id);
	}else if(strstr(data, "LED_ON") != NULL){
		//LED_ON
		ShareData_SetReg(4, 1);
		wifi_send_via_tcp("LED_ON_OK\r\n", 11, link_id);
	}else if(strstr(data, "GET_TEMP") != NULL){
		char reply[32];
    sprintf(reply, "TEMP:%d\r\n", ShareData_GetReg(1));
		wifi_send_via_tcp(reply, strlen(reply), link_id);
	}else{
	  wifi_send_via_tcp("UNKNOWN\r\n", 9, link_id);
	}
}


/**
 * @brief WIFI状态机
 * @param data数组指针
 * @param 连接ID
 * @note 收到的数据做对应的动作，并将成功的结果返回给TCP客户端
 */
static void wifi_state_machine(char *line){
	printf("[SM] state=%d, line=%s\r\n", wifi_state, line);  // 打印当前状态和行内容
  switch(wifi_state){
	  case WIFI_IDLE:
			//空闲状态：检查是否有+IPD（TCP 数据到达）
		  if(strstr(line, "+IPD") != 0){
				//解析连接ID
				//+IPD,1,7,:LED_ON  //+IPD,0,8:LED_OFF
		    char *p = strstr(line, "+IPD,");
				if(p != NULL){
				  p = p + 5;
					uint8_t link_id = atoi(p);//提取数字

					char *data = strstr(line, ":");//取冒号后面的内容
					if(data != NULL){
					  data++;
					  wifi_state_start_time = HAL_GetTick();
					  handle_tcp_command(data , link_id);//执行动作+回复
					}
				}
		  }
			break;
			
		case WIFI_WAITING_OK:
			//等待OK响应
		  if(strstr(line, "OK") != NULL){
			  wifi_state = WIFI_IDLE;
				wifi_cmd_success = 1;
				printf("[[WIFI] Command OK\r\n");
			}else if(strstr(line, "ERROR") != NULL){
			  wifi_state = WIFI_IDLE;
				wifi_cmd_success = 0;
				printf("[WIFI] Command ERROR\r\n");
			}else{
			  wifi_state = WIFI_IDLE;
				wifi_cmd_success = 0;
				printf("[WIFI] Unexpected reply, reset to IDLE: %s\r\n", line);
			}
			break;
			
		case WIFI_WAITING_DATA:
			wifi_state = WIFI_IDLE;
			break;
		
		case WIFI_WAITING_PROMPT:
			if(strstr(line, ">") != NULL || strstr(line, "OK")){
			  // 收到 >，可以发数据
				wifi_send_raw(tcp_pending_data);
				printf("[TCP] tcp_pending_data: %s\r\n", tcp_pending_data);
				wifi_state = WIFI_IDLE;
				wifi_cmd_success = 1;
			}else if(strstr(line, "busy") != NULL || strstr(line, "BUSY") != NULL){
			  //ESP-01S正在忙重置计数器
				wifi_state_start_time = HAL_GetTick();
				printf("[TCP] ESP-01S busy, reset timer and wait...\r\n");
				
			}else if(strstr(line, "ERROR") != NULL || strstr(line, "SEND FAIL") != NULL){
        //发送失败
			  wifi_state = WIFI_IDLE;
				wifi_cmd_success = 0;
				printf("[TCP] Send failed: %s\r\n", line);
			}else if (strstr(line, "Recv") != NULL){
			  //ESP-01S确认收到数据，保持状态，继续等待
				printf("[TCP] ESP-01S confirmed receive: %s\r\n", line);
			}else{
			  // 如果收到其他行，不处理，保持状态等待 '>'
				printf("[TCP] Waiting for '>'... got %s\r\n", line);
			}
		 break;
	}
}

/**
 * @brief TCP 发送数据
 * @param data数组指针
 * @param 连接ID
 * @note 发送数据给TCP客户端
 */
static void wifi_send_via_tcp(char *data, uint16_t len, uint8_t link_id){
	//状态机不空闲，不发送
  if(wifi_state != WIFI_IDLE){
	  printf("[TCP] Busy, drop reply: %s", data);
		return;
	}
	
	strncpy(tcp_pending_data, data, sizeof(tcp_pending_data) -1);
	tcp_pending_data[sizeof(tcp_pending_data) -1 ] = '\0';

	//发CIPSEND指令
	char cmd[32];
	sprintf(cmd, "AT+CIPSEND=%d,%d\r\n", link_id, len);
	printf("[TCP] CMD: %s", cmd);
	wifi_send_raw(cmd); //发送 CIPSEND 指令,ESP01需要返回'>'提示符后再发指令

	//切换状态，等待'>'提示符
	wifi_state = WIFI_WAITING_PROMPT;
	wifi_state_start_time = HAL_GetTick();
	wifi_cmd_success =0;
}

/**
 * @brief WIFI复位函数
 * @note 重置状态机、软复位
 */
static void wifi_force_reset(void){
  printf("[WIFI] Force reset started...\r\n");
	//1 清除状态
	wifi_state = WIFI_IDLE;
	wifi_init_flag = 0;
//	wifi_connected = 0;
	
	//2 清空环形缓冲区
	wifi_rx_ring.read = wifi_rx_ring.write;
	
	//3 发送软复位指令
	wifi_send_raw("AT+RST\r\n");
	
	//4 等待模块重启
	vTaskDelay(pdMS_TO_TICKS(3000));

	//5 重新初始化
	if(Wifi_AT_Init() == 1){
	  printf("[WIFI] Force reset success!\r\n");
	}else{
	  printf("[WIFI] Force reset FAILED, need manual check.\r\n");
	}
}

/**
 * @brief 串口中断数据保存
 * @param 数组指针data
 * @note 将数据保存进缓冲区
 */
void wifi_rx_push(uint8_t data){
  uint16_t next = (wifi_rx_ring.write + 1) % WIFI_RING_BUF_SIZE;
	
	//缓存区满，读指针加1
	if(next == wifi_rx_ring.read){
	  next = (wifi_rx_ring.read + 1) % WIFI_RING_BUF_SIZE;
	}
	
	wifi_rx_ring.buffer[wifi_rx_ring.write] = data;
	wifi_rx_ring.write = next;
}

/**
 * @brief 串口数据读取
 * @param 数组指针data
 * @note 从缓冲区取数据，每次一个字节
 */
static uint8_t wifi_rx_pop(uint8_t *data){
  if(wifi_rx_ring.write == wifi_rx_ring.read) return 0;
	
	*data = wifi_rx_ring.buffer[wifi_rx_ring.read];
	wifi_rx_ring.read = (wifi_rx_ring.read + 1)% WIFI_RING_BUF_SIZE;
	return 1;
}

#if 0
//解析wifi数据
void wifi_parse_line(char *line){
	static uint8_t wifi_last_cmd_ok;
	
  //1 检查是否为 +IPD
	// +IPD 是 ESP-01S 主动上报的数据，格式固定为：
	// +IPD,<连接ID>,<数据长度>:<实际数据>
  if(strstr(line, "+IPD") != NULL){
	  char *data = strstr(line, ":");
		if(data != NULL){
		  data++; //跳过冒号
			handle_tcp_command(data);
		}
	}
	
	//2 检查是否为 OK/ERROR
	if(strstr(line, "OK") != NULL){
	  wifi_last_cmd_ok = 1;
	}
	if(strstr(line, "ERROR") != NULL){
    wifi_last_cmd_ok = 0;
	}
}

//发指令，等待回复，超时会退出，原始数据存到resp
uint8_t wifi_send_cmd_test(const char *cmd, char *resp, uint16_t resp_size, uint32_t timeout_ms){
  uint16_t len = strlen(cmd);
	uint32_t start = HAL_GetTick();
	uint8_t rx_len = 0;
	uint8_t c;
	uint8_t wifi_rx_buf[128];
	uint32_t last_byte_time = HAL_GetTick();
	
	//1 清空接收缓冲区
	memset(wifi_rx_buf, 0, sizeof(wifi_rx_buf));
	
	//2 发送指令
  if(!wifi_send_raw(cmd))return 0;
	printf("[WIFI] Send: %s", cmd);
	
	//3 接收回复,50ms无新数据就退出
	while(1){
	/*轮询读硬件方法	
	  //检查是否有数据
		if(__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) != RESET){
		  HAL_UART_Receive(&huart2, &c, 1, 10);
			if(rx_len < sizeof(wifi_rx_buf) - 1){
			  wifi_rx_buf[rx_len++] = c;
			}
			//收到数据后，重置计数
			last_byte_time  = HAL_GetTick();
		}
	*/

		//环形缓冲区读取数据
		if(wifi_rx_pop(&c)){
		  if(rx_len < sizeof(wifi_rx_buf) -1){
			  wifi_rx_buf[rx_len++] = c;
#if WIFI_DEBUG_ENABLE
        printf("[RAW] 0x%02X ", c);
#endif
			}
      last_byte_time  = HAL_GetTick();
		}

		//如果超过50ms，没有新数据认为回复结束
		if((HAL_GetTick() - last_byte_time) > 50){
			  break;
		}
		
		//整体超时保护
		if(HAL_GetTick() - start > timeout_ms){
		  break;
		}
	}

	//4 复制结果
	if(rx_len > 0 && resp && resp_size > 0){
		printf("\r\n[WIFI] Recv: %s\r\n", wifi_rx_buf);
		  memcpy(resp, wifi_rx_buf, (rx_len > resp_size)?rx_len: resp_size -1);
			resp[(rx_len < resp_size)? rx_len : resp_size - 1] = '\0';
	}else{
	  printf("[WIFI] Recv: TIMEOUT\r\n");
	}

  return rx_len;
}

void wifi_test(void){
  char resp[64] = {0};
	printf("\r\n=== WiFi Test Start ===\r\n");
	
	//测试AT指令
	if(wifi_send_cmd_test("AT\r\n", resp, sizeof(resp), 500) > 0){
		//检查是否包含OK
	  if(strstr(resp, "OK") != NULL){
		  printf("[WIFI] AT command OK! Module is alive.\r\n");
		}else{
		  printf("[WIFI]  Unexpected response: %s\r\n", resp);
		}
	}else{
	  printf("[WIFI]  No response (timeout). Check wiring!\r\n");
	}
	
	if(wifi_send_cmd_test("AT+RST\r\n", resp, sizeof(resp), 2000) > 0){
	  if(strstr(resp, "OK") != NULL){
		  printf("[WIFI] AT+RST command OK! Module is alive.\r\n");
			printf("[WIFI] AT+RST Rec :%s", resp);
		}
	}
	if(wifi_send_cmd_test("AT+CIPSTATUS\r\n", resp, sizeof(resp), 2000) > 0){
		printf("[WIFI] AT+CIPSTATUS Rec :%s", resp);
	}
	
	printf("=== WiFi Test End ===\r\n\r\n");
}
#endif
