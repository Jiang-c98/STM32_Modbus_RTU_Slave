/**
 * @file    bluetooth_jdy31.c
 * @brief   BT模块
 * @author  Cui Jiang
 * @date    2025-08-15
 */

/* 头文件包含 */
#include "User/bluetooth_jdy31.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "User/share_data.h"
#include "usart.h"

/* 私有宏定义 */
#define RING_BUF_SIZE 128

/* 全局变量 */
uint8_t BT_byte;

//环形缓冲区
typedef struct{
  uint8_t buffer[RING_BUF_SIZE];
	volatile uint16_t read;
	volatile uint16_t write;
}ring_buffer_t;

/* 私有变量 */
static ring_buffer_t bt_rx_ring;
static char bt_line_buf[RING_BUF_SIZE]; //解析行缓冲区
static uint8_t bt_line_len = 0;         //行数据长度
static uint8_t bt_line_ready = 0;       //完整行标志位

/* 私有函数声明 */
static void BT_process(void);
static void BT_HandleData(char *data);

//初始化
void BT_Init(void){
	HAL_UART_Receive_IT(&huart2, &BT_byte, 1);
}

/**
 * @brief BT任务
 * @note 进入循环检查缓冲区完整行，解析
 */
static void vBT_Task(void *pvParameters){
  for(;;){
	  //保存完整行
		BT_process();
//	  printf("[BT] BT_process bt_line_ready=%d\r\n",bt_line_ready);
		
		if(bt_line_ready){
			printf("[BT]bt_line_buf = %s \r\n", bt_line_buf);
			bt_line_ready = 0;
			BT_HandleData(bt_line_buf);
		}
		vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/**
 * @brief BT任务
 * @note 启动BT任务
 */
void StartBTTask(void){
	//创建BT主任务
	if(xTaskCreate(vBT_Task, "BT", 256, NULL, 2, NULL) != pdPASS){
	  printf("vBT_Task create failed!\r\n");
	}
}

/**
 * @brief 环形缓冲区取数据，拼成一行
 * @note 判断是否有数据，末尾字符是否为'\n'换行，拼接'\0'结束符
 */
static void BT_process(void){
	uint8_t c;
  while(bt_rx_pop(&c)){
		//非空行
	  if(c == '\n'){
		  if(bt_line_len > 0 ){
				bt_line_buf[bt_line_len] = '\0';
				bt_line_ready = 1;
				bt_line_len = 0;
				printf("[BT] BT_process:%s\r\n", bt_line_buf);
			}
		}else if(c != '\r' && bt_line_len < sizeof(bt_line_buf) - 1){
		  bt_line_buf[bt_line_len++] = c;
		}
	}
}

/**
 * @brief 发送数据函数
 * @note 发送数据
 */
static uint8_t BT_SendData(const char *data, uint16_t len){
  if(HAL_UART_Transmit(&huart2, (uint8_t *)data, len, 100) != HAL_OK){
	  printf("[BT] BT_SendData fail!");
		return 0;
	}
	return 1;
}

/**
 * @brief 应用层处理,解析数据
 * @note 处理蓝牙接收到数据
 */
static void BT_HandleData(char *data){
	//数据采集指令
	printf("[BT] Received: '%s'\r\n", data);
	if(strstr(data, "LED_OFF") != NULL){
	  ShareData_SetReg(4, 0);
		BT_SendData("LED_OFF_OK\r\n", 12);
	}else if(strstr(data, "LED_ON") != NULL){//LED_ON
	  ShareData_SetReg(4, 1);
		BT_SendData("LED_ON_OK\r\n", 11);
	}else if(strstr(data, "GET_TEMP") != NULL){//LED_OFF
		char reply[32];
    sprintf(reply, "TEMP:%d.%d C\r\n", ShareData_GetReg(2) / 10, ShareData_GetReg(2) % 10);
		BT_SendData(reply, strlen(reply));
	}else if(strstr(data, "servo") != NULL){
		char *p =strstr(data, ":");
		char reply[32];
		uint8_t angle = 0;
		if(p != NULL){
			p++;
			//把:后面的字符串转为数字
		  angle = atoi(p);
			ShareData_SetReg(5, angle);
			//限幅180度，超过的时候固定在180
			sprintf(reply, "servo:%d\r\n", angle);
		  BT_SendData(reply, strlen(reply));
		}
	}else if(strstr(data, "stepper") != NULL){
		char *p = strstr(data, ":");
		char reply[32];
		uint8_t cmd = 0;
		if(p != NULL){
		  p++;
			//ASCII 字符转换成数字
			cmd = p[0] - '0';//'1' → 1, '2' → 2
			ShareData_SetReg(6, cmd);
			sprintf(reply, "stepper:%d\r\n", cmd);
		  BT_SendData(reply, strlen(reply));
		}
	}else{
		BT_SendData("UNKNOWN\r\n", 9);
	}
}

/* 硬件层:串口接收 */
//串口中断数据保存
void bt_rx_push(uint8_t data){
  uint16_t next = (bt_rx_ring.write + 1) % RING_BUF_SIZE;
	
	if(next == bt_rx_ring.read){//满了
	  next = (bt_rx_ring.read + 1) % RING_BUF_SIZE;
	}
	
	bt_rx_ring.buffer[bt_rx_ring.write] = data;
	bt_rx_ring.write = next;
}

//串口数据读取
uint8_t bt_rx_pop(uint8_t *data){
  if(bt_rx_ring.write == bt_rx_ring.read) return 0;
	*data = bt_rx_ring.buffer[bt_rx_ring.read];
	bt_rx_ring.read = (bt_rx_ring.read + 1)% RING_BUF_SIZE;
	return 1;
}
