#include "User\oled.h"
#include "User\key.h"
#include "User\rs485_modbus.h"
#include "string.h"

#define OLED_ADDR 0x3C 
#define OLED_WRITE_ADDR (0x3C << 1)  

extern I2C_HandleTypeDef hi2c1;
extern uint8_t mainMenuSelection;
u8g2_t u8g2;

static void OLED_ShowTempInfo(uint16_t temp);
static void OLED_ShowLightTest(uint16_t percent);
static void OLED_ShowMenu(uint8_t hand);
static void Manual_Sendbuffer(u8g2_t *u8g2);

//OLED初始化
void OLED_Init(void){
  printf("OLED_Init...\r\n");
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
	
	u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0,  My_U8x8_I2c_HwSend,  My_gpio_and_delay_cb);
	u8g2_InitDisplay(&u8g2);
	u8g2_ClearBuffer(&u8g2);
	Manual_Sendbuffer(&u8g2);	
}

//UI显示函数
void OLED_UpdateDisplay(MenuState_t menustate){
  switch(menustate){
	  case STATE_MENU:
			OLED_ShowMenu(mainMenuSelection);
			break;
		
		case STATE_LIGHT_SENSOR:
			OLED_ShowLightTest(RS485_Modbus_GetReg(0));
			break;
		
		case STATE_TEMP_SENSOR:
			OLED_ShowTempInfo(RS485_Modbus_GetReg(1));
			break;
	}
}

//光敏测试界面
static void OLED_ShowLightTest(uint16_t percent){
  char buf[32];
	u8g2_ClearBuffer(&u8g2);//清理ram上的显存镜像空间
	u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);//设置字体
	
	u8g2_DrawStr(&u8g2, 0, 10, "Light Test");//字体本上有高度，所以需要从10开始
	
	snprintf(buf, sizeof(buf), "Light value:%d", percent);//拼接字符换后传递给buf数组
	u8g2_DrawStr(&u8g2, 0, 26, buf);//从第一列开始，16高度下开始

  u8g2_DrawStr(&u8g2, 0, 42, "Press RESET exit!");
	Manual_Sendbuffer(&u8g2);
}

//热敏测试界面
static void OLED_ShowTempInfo(uint16_t temp){
  char buffer[32];
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
	
	u8g2_DrawStr(&u8g2, 0, 10, "Temp Test:");
	
	snprintf(buffer, sizeof(buffer), "Temp: %d", temp);
	u8g2_DrawStr(&u8g2, 0, 26, buffer);
	
//	snprintf(buffer, sizeof(buffer), "Thresh: %d",threshold);
//	u8g2_DrawStr(&u8g2, 0, 42, buffer);
	
	u8g2_DrawStr(&u8g2, 0, 42, "Press RESET exit");
	Manual_Sendbuffer(&u8g2);
}

//主菜单界面
static void OLED_ShowMenu(uint8_t hand)
{
		u8g2_ClearBuffer(&u8g2);
		u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
		if(hand == 0){
		  u8g2_DrawStr(&u8g2, 0, 10, ">  1. Light Test");
		  u8g2_DrawStr(&u8g2, 0, 26, "     2. Temp Test");
		}else if(hand == 1){
		  u8g2_DrawStr(&u8g2, 0, 10, "     1. Light Test");
		  u8g2_DrawStr(&u8g2, 0, 26, ">  2. Temp Test");
		}else if(hand == 2){
			u8g2_DrawStr(&u8g2, 0, 10, "     1. Light Test");
		  u8g2_DrawStr(&u8g2, 0, 26, "     2. Temp Test");
		}
		Manual_Sendbuffer(&u8g2);
}


//u8g2回调函数，但是存在问题，使用手动发送方式
uint8_t My_U8x8_I2c_HwSend(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
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
		  }	
			break;
		
		default:
			break;
	}
	return 1;
}

//u8g2注册回调函数时的延时函数
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

//手动发送到OLED显存
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

