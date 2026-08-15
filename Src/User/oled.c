/**
 * @file    oled.c
 * @brief   Oled模块
 * @author  Cui Jiang
 * @date    2025-06-07
 */

/* 头文件包含 */
#include "User/oled.h"
#include <string.h>
#include "User/key.h"
#include "User/share_data.h"
#include "i2c.h"

/* 私有宏定义 */
#define OLED_ADDR 0x3C 
#define OLED_WRITE_ADDR (0x3C << 1)  
#define OLED_REFRESH_INTERVAL_MS  10

/* 外部变量声明 */
extern I2C_HandleTypeDef hi2c1;
extern uint8_t mainMenuSelection;

u8g2_t u8g2;

/* 私有函数声明 */
static void OLED_UpdateDisplay(MenuState_t menustate);
static void HanldeKeyEvent(KeyEvent_t *key);
static void OLED_ShowTempInfo(uint16_t temp);
static void OLED_ShowLightTest(uint16_t percent);
static void OLED_ShowDHT22info(void);
static void OLED_ShowMenu(uint8_t hand);
static void Manual_Sendbuffer(u8g2_t *u8g2);

/**
 * @brief OLED显示任务
 * @note  OLED显示任务
          OLED 刷新率分析（2026-08-15）
          - I2C 400kHz 下，传输 1024 字节约需 29ms
          - 加上任务调度开销，实际刷新率约 14~15Hz
          - 人眼观看稳定（>12Hz），相机拍摄时（快门速度 1/50s 以上）会出现闪烁
          - 如需消除拍照闪烁，建议更换 SPI 接口 OLED 或实现增量刷新
 */
void vOLed_Task(void *pvParameters){	
	//调试阶段,任务入口打印栈余量
	//UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
  KeyEvent_t key_receive;
	uint32_t last_refresh_time;
	for(;;){
    if(xQueueReceive(xKeyQueue, &key_receive, pdMS_TO_TICKS(5)) == pdTRUE){
		  //状态机处理
			HanldeKeyEvent(&key_receive);
			OLED_UpdateDisplay(currentMenu);
			last_refresh_time = HAL_GetTick();
		}

		if(HAL_GetTick() - last_refresh_time > OLED_REFRESH_INTERVAL_MS){
      //uint32_t start = HAL_GetTick();
		  OLED_UpdateDisplay(currentMenu);
      //uint32_t elsped = HAL_GetTick() - start;
			//i2c发数据需要33ms 1000/33 =30.3，start两次间隔无法满足30Hz要求，手机拍摄会闪烁
      //printf("OLED refresh time: %d  start=%d\r\n", elsped, start);
			last_refresh_time = HAL_GetTick();
		}
	  vTaskDelay(pdMS_TO_TICKS(200));
	}
}

/**
 * @brief OLED任务
 * @note 启动OLED功能
 */
void StartOledTask(void){
	//创建OLED任务
  if(xTaskCreate(vOLed_Task, "Oled", 256, NULL, 2, NULL) != pdPASS){
	  printf("vOLED_Task create failed\r\n");
	}
}

/**
 * @brief 按键菜单状态机
 * @note  根据按键切换状态
 */
static void HanldeKeyEvent(KeyEvent_t *key){
  switch(currentMenu){
	  case STATE_MENU:
			if(key->key_event == 0){
				mainMenuSelection = (mainMenuSelection + 1) % 3;

			}else if(key->key_event == 1){
				if(mainMenuSelection == 0){
				  currentMenu = STATE_LIGHT_SENSOR;
				}else if(mainMenuSelection == 1){
				  currentMenu = STATE_TEMP_SENSOR;
				}else{
				  currentMenu = STATE_DHT22_SENSOR;
				}
//			  currentMenu = (mainMenuSelection == 0)?STATE_LIGHT_SENSOR:STATE_TEMP_SENSOR;
			}
			break;

		case STATE_LIGHT_SENSOR:
		case STATE_TEMP_SENSOR:
		case STATE_DHT22_SENSOR:
			if(key->key_event == 1){
			  currentMenu = STATE_MENU;
			}
			break;
	}
}

/**
 * @brief OLED 初始化
 * @note 通过I2C总线将初始化参数按照命令字节+参数格式发送
 */
void OLed_Init(void){
  printf("OLED_Init...\r\n");
	//方法1:初始化参数
  uint8_t init_cmds[] = {
			0xAE,       // 关闭显示
			0xD5, 0x80, // 时钟分频
			0xA8, 0x3F, // 多路复用率（64行）
			0xD3, 0x00, // 显示偏移
			0x40,       // 起始行
			0x8D, 0x14, // 电荷泵使能（关键！）
			0xA1,       // 段重映射
			0xC8,       // COM 扫描方向
			0xDA, 0x12, // COM 引脚配置
			0x81, 0xCF, // 对比度
			0xD9, 0xF1, // 预充电周期
			0xDB, 0x40, // VCOM 电压
		  0x20, 0x02, // 【关键】设置为页寻址模式 (Page Addressing Mode)
			0xA4,       // 全局显示开启
			0xA6,       // 正常显示（非反色）
			0xAF        // 开启显示
	};
	for(uint8_t i = 0; i < sizeof(init_cmds); i++)
	{
	  uint8_t buf[2] = {0x00, init_cmds[i]};
		if(HAL_I2C_Master_Transmit(&hi2c1, OLED_WRITE_ADDR, buf, 2, 100) != HAL_OK)
		{
		   printf("HAL_I2C_Master_Transmit failed!...\r\n");
		}

		HAL_Delay(1);
	}
	
	//注册回调函数，但是Sendbuffer存在问题，使用了
	u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0,  My_U8x8_I2c_HwSend,  My_gpio_and_delay_cb);
	//	//方法二:u8g2库函数初始化，长时间断点重新上电无显示
//	u8g2_InitDisplay(&u8g2);
	//清理u8g2本地映射的显存空间
	u8g2_ClearBuffer(&u8g2);
	//手动发送buffer给oled的显存空间
	Manual_Sendbuffer(&u8g2);	
}

/**
 * @brief UI显示函数
 * @param menustate 菜单状态机
 * @note 由菜单状态机决定当前显示界面
 */
static void OLED_UpdateDisplay(MenuState_t menustate){
  switch(menustate){
	  case STATE_MENU:
			//主菜单
			OLED_ShowMenu(mainMenuSelection);
			break;

		case STATE_LIGHT_SENSOR:
			//光敏测试，获取寄存器值
			OLED_ShowLightTest(ShareData_GetReg(0));
			break;

		case STATE_TEMP_SENSOR:
			//热敏测试
			OLED_ShowTempInfo(ShareData_GetReg(1));
			break;

		case STATE_DHT22_SENSOR:
			//DHT22 温湿度数据
		  OLED_ShowDHT22info();
			break;
	}
}

/**
 * @brief 光敏测试界面
 * @param percent 传感器数据
 */
static void OLED_ShowLightTest(uint16_t percent){
  char buf[32];
	u8g2_ClearBuffer(&u8g2);//清理ram上的显存镜像空间
	u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);//设置字体

	u8g2_DrawStr(&u8g2, 0, 10, "Light Test");//字体本上有高度，所以需要从10开始

	snprintf(buf, sizeof(buf), "Light value:%d", percent);//拼接字符换后传递给buf数组
	u8g2_DrawStr(&u8g2, 0, 26, buf);//从第一列开始，16高度下开始

  u8g2_DrawStr(&u8g2, 0, 42, "Long press to exit!");
	Manual_Sendbuffer(&u8g2);
}

/**
 * @brief 热敏测试界面
 * @param percent 传感器数据
 */
static void OLED_ShowTempInfo(uint16_t temp){
  char buffer[32];
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
	
	u8g2_DrawStr(&u8g2, 0, 10, "Temp Test:");
	
	snprintf(buffer, sizeof(buffer), "Temp: %d", temp);
	u8g2_DrawStr(&u8g2, 0, 26, buffer);
	
//	snprintf(buffer, sizeof(buffer), "Thresh: %d",threshold);
//	u8g2_DrawStr(&u8g2, 0, 42, buffer);
	
	u8g2_DrawStr(&u8g2, 0, 42, "Long press to exit");
	Manual_Sendbuffer(&u8g2);
}

static void OLED_ShowDHT22info(){
  char buffer[32];
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);

	//温度
	u8g2_DrawStr(&u8g2, 0, 10, "DHT22 Test:");
	uint16_t temp_int = ShareData_GetReg(2) / 10;
	uint16_t temp_float = ShareData_GetReg(2) % 10;
	snprintf(buffer, sizeof(buffer), "Temp: %d.%d C",temp_int, temp_float);
	u8g2_DrawStr(&u8g2, 0, 26, buffer);

	//湿度
	uint16_t humidity_int = ShareData_GetReg(3) /10;
	uint16_t humidity_float = ShareData_GetReg(3) %10;
	snprintf(buffer, sizeof(buffer), "humidity: %d.%d %%",humidity_int, humidity_float);
	u8g2_DrawStr(&u8g2, 0, 42, buffer);

	u8g2_DrawStr(&u8g2, 0, 58, "Long press to exit");
	Manual_Sendbuffer(&u8g2);
}


/**
 * @brief 主菜单界面
 * @param hand 实际指针箭头
 */
static void OLED_ShowMenu(uint8_t hand)
{
		u8g2_ClearBuffer(&u8g2);
		u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
		if(hand == 0){
		  u8g2_DrawStr(&u8g2, 0, 10, ">  1. Light Test");
		  u8g2_DrawStr(&u8g2, 0, 26, "     2. Temp Test");
			u8g2_DrawStr(&u8g2, 0, 42, "     3. DHT22 Test");
		}else if(hand == 1){
		  u8g2_DrawStr(&u8g2, 0, 10, "     1. Light Test");
		  u8g2_DrawStr(&u8g2, 0, 26, ">  2. Temp Test");
			u8g2_DrawStr(&u8g2, 0, 42, "     3. DHT22 Test");
		}else if(hand == 2){
			u8g2_DrawStr(&u8g2, 0, 10, "     1. Light Test");
		  u8g2_DrawStr(&u8g2, 0, 26, "     2. Temp Test");
			u8g2_DrawStr(&u8g2, 0, 42, ">  3. DHT22 Test");
		}
		Manual_Sendbuffer(&u8g2);
}

/**
 * @brief u8g2库 i2c 硬件传输回调函数
 * @param u8x8 u8g2底层结构体指针
 * @param msg  消息类型，决定本次调用的动作
 *                - U8X8_MSG_BYTE_INIT: 初始化，重置缓存索引
 *                - U8X8_MSG_BYTE_START_TRANSFER: 开始一次传输，重置缓存索引
 *                - U8X8_MSG_BYTE_SEND: 收到数据/命令，先存入缓存区
 *                - U8X8_MSG_BYTE_END_TRANSFER: 传输结束，打包并发送 I2C 数据
 * @param arg_int 数据长度
 * @param arg_ptr 数据指针，指向要发送的字节
 * @retval 1 固定返回成功
 * @note 本函数使用静态缓存区将多次u8g2合并为一次I2C发送
 *       根据第一个字节判断判断命令（0x00）还是数据（0x40）
 */
uint8_t My_U8x8_I2c_HwSend(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	//u8g2库i2c回调函数，但是存在问题，目前使用的都是手动发送方式
  static uint8_t buf[128];//缓存空间
	static uint8_t buf_idx = 0;//缓存指引
	uint8_t *data;
	switch(msg)
	{
	  case U8X8_MSG_BYTE_INIT:
			buf_idx = 0;
		  break;
		
		case U8X8_MSG_BYTE_START_TRANSFER:
			buf_idx = 0;
		  break;
		
		case U8X8_MSG_BYTE_SEND://整合到buf中，多了一个字节放置控制指令（0x00命令，0x40数据）
			data = (uint8_t *)arg_ptr;//将arg_ptr指针指向data指针指向的内容
		    if(buf_idx + arg_int <= sizeof(buf[128]))//发了arg_int长度的字节，全部放进缓存空间
        {
			    memcpy(&buf[buf_idx], data, arg_int);
				  buf_idx = buf_idx + arg_int;
			  }
			break;
		
		case U8X8_MSG_BYTE_END_TRANSFER:
		{	
		  uint8_t send_buf[buf_idx + 1];
      //命令的特征：在 U8g2 中，所有命令字节都是 小于 0x80 的，而且肯定不是 0x40（因为 0x40 是有特殊用途的控制字节）。
      //数据的特征：显示数据通常是 0x80 ~ 0xFF 之间的值（像素数据），或者偶尔是 0x40 这个特殊值
			if((buf[0] < 0x80)&&(buf[0] != 0x40))
			{
			  send_buf[0] = 0x00;
			}else{
			  send_buf[0] = 0x40;
			}
			memcpy(&send_buf[1], buf, buf_idx);//send_buf第一个数组成员是0x00或者0x40
			HAL_I2C_Master_Transmit(&hi2c1, OLED_WRITE_ADDR, send_buf, buf_idx + 1, 100);
			break;
		}	
		
		default:
			break;
	}
	return 1;
}

/**
 * @brief u8g2库的延时和GPIO控制回调函数（硬件抽象层）
 * @param u8x8 指向u8g2底层结构体指针
 * @param msg  消息类型，U8X8_MSG_DELAY_MILLI 表示毫秒延时
 * @param arg_int 延时毫秒数
 * @param arg_ptr 消息的指针参数（未使用）
 * @retval 1 固定返回成功
 */
uint8_t My_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg)
	{	
    case U8X8_MSG_DELAY_MILLI:
			HAL_Delay(arg_int);//根据函数传进来arg_int的延时试尝决定
      break;
		
    case U8X8_MSG_DELAY_10MICRO:
      break;
		
    case U8X8_MSG_DELAY_100NANO:
      break;
		
    case U8X8_MSG_DELAY_NANO:
      break;
	}
	return 1;
}

/**
 * @brief 手动发送到OLED显存
 * @param u8g2 指向u8g2底层结构体指针，可用于获取本地缓冲区地址
 * @note 本函数通过页寻址方式0xB0开始，设置先低后高，发送数据到OLED显存
 *       1.发送页地址命令（0xB0~0xB7）
 *       2.发送列地址（低4位和高四位，从0列开始）
 *       3.连续发送128个字节数据（对应当前页的一整行）
 *       4.数据包格式；[0x40]+[128字节现存数据]共129个字节
 */
static void Manual_Sendbuffer(u8g2_t *u8g2)
{
	uint8_t tx_buf[129];
	uint8_t *data = (uint8_t *)u8g2->tile_buf_ptr;
	tx_buf[0] = 0x40;
	for(uint8_t page = 0; page < 8; page++)//页地址扫描
  {
		uint8_t page_cmd[2] = {0x00, 0xB0 + page};
	  HAL_I2C_Master_Transmit(&hi2c1, OLED_WRITE_ADDR, page_cmd, 2, 100);//写页地址
		
		uint8_t col_low_cmd[2] = {0x00, 0x00};//0x00~0x0F 低四位地址
		uint8_t col_high_cmd[2] = {0x00, 0x10};//0x10~0x1F高四位地址
		HAL_I2C_Master_Transmit(&hi2c1, OLED_WRITE_ADDR, col_low_cmd, 2, 100);
		HAL_I2C_Master_Transmit(&hi2c1, OLED_WRITE_ADDR, col_high_cmd, 2, 100);
    
		for(uint8_t col = 0; col < 128; col++)//对应字模数据列行式发送
		{
			tx_buf[1+col] = data[page*128+col];
			//执行一些字符串显示时，u8g2结构体的tile_buf_ptr指针会指向字符串的内存空间首地址
		}
    HAL_I2C_Master_Transmit(&hi2c1, OLED_WRITE_ADDR, tx_buf, 129, 100);
    //发送129字节(包括0x40数据包头， 数据控制字节)
	}
}

