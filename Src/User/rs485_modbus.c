/**
 * @file    rs485_modbus.c
 * @brief   RS485、Modbus协议通信模块
 * @author  Cui Jiang
 * @date    2025-06-07
 */

/* 头文件包含 */
#include "User/rs485_modbus.h"
#include "User/share_data.h"

//Modbus
//CRC校验查表法
uint8_t auchCRCHi[]=
{
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40
};
 
uint8_t auchCRCLo[] =
{
0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
0x40
};

/* 私有变量声明 */
static uint8_t Modbus_RxData[RX_BUF_SIZE];
static volatile uint16_t Modbus_rxWrite_index;//缓存区写指针
static volatile uint16_t Modbus_rxRead_index;//缓存区读指针
static uint32_t last_rx_time;

/* 全局变量 */
uint8_t Modbus_byte;

/* 私有函数声明 */
static void BuildExceptionResponse(uint8_t *frame, uint8_t errcode);
static void Modbus_SendResponse(uint8_t *frame, uint8_t len);
static void RS485_SetMode(uint8_t rw_status);
static void RS485_Modbus_SetReg(uint16_t addr, uint16_t value);
static void RS485_Modbus_GetReg(uint16_t addr, uint16_t *value);
static void RS485_Modbus_Time(void);

/**
 * @brief Modbus任务
 * @note Modbus任务
 */
void vModbus_Task(void *pvParameters){
  for(;;){
	  RS485_Modbus_Time();
    vTaskDelay(pdMS_TO_TICKS(10));
	}
}

/**
 * @brief Modbus任务
 * @note 启动Modbus功能
 */
void StartModbusTask(void){
	//创建Modbus任务
	if(xTaskCreate(vModbus_Task, "modbus", 512, NULL, 2, NULL) != pdPASS){
		printf("vModbus_Task create failed\r\n");
	}
}

/**
 * @brief 初始化
 * @note 设置读RS485状态、串口中断开始接收
 */
void Modbus_Init(void){
	RS485_SetMode(0);
  HAL_UART_Receive_IT(&huart3, &Modbus_byte, 1);
}

/**
 * @brief RS485读写控制函数
 * @param 0为读状态，1为写状态
 */
static void RS485_SetMode(uint8_t rw_status){
  if(rw_status == 0){
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
	}else{
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
	}
}

/**
 * @brief 读写保持寄存器值
 * @param addr 地址
 * @param value 变量
 * @note  调用过共享数据的接口写入或者读出
 */
static void RS485_Modbus_SetReg(uint16_t addr, uint16_t value){
  //调用共享数据的接口,写入数据
	ShareData_SetReg(addr, value);
}
static void RS485_Modbus_GetReg(uint16_t addr, uint16_t *value){
  //共享数据接口，读出
  *value = ShareData_GetReg(addr);
}

/**
 * @brief CRC检验
 * @param buffer 数据指针
 * @param length 数据长度
 * @return CRC16 值
 */
static uint16_t CRC16(uint8_t *buffer, uint16_t len){
  uint8_t crcHigh = 0xFF;
	uint8_t crcLow = 0xFF;
	uint8_t index;
	while(len--){
		//第一次0xFF和第一个数据异或
		//第二次用上一次的crcHigh和buffer第二个数据异或
    index = crcHigh ^ *buffer;
		buffer++;
		//0xFF和CRC高字节表里的值异或
		crcHigh = crcLow ^ auchCRCHi[index];
		//低位直接查低字节表
		crcLow = auchCRCLo[index];
	}
	return (crcHigh << 8) | crcLow;
}

/**
 * @brief 串口3中断函数
 * @param huart 串口指针
 * @return CRC16 值
 * @note 调用逻辑，USART3_IRQHandler-->HAL_UART_IRQHandler-->
 * UART_Receive_IT(huart)-->HAL_UART_RxCpltCallback(huart)
 * @note 环形缓冲区方式，写指针存储串口接收的数据
 */
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
//  if(huart->Instance == USART3){
//		HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_5);
//	  //中间变量next_index
//		uint16_t next_index = (Modbus_rxWrite_index + 1) % RX_BUF_SIZE;
//		
//		//防止写超过读指针
//		if(next_index == Modbus_rxRead_index){
//		  //环形缓冲方式 读指针+1
//			Modbus_rxRead_index = (Modbus_rxRead_index + 1) % RX_BUF_SIZE;
//		}
//		
//		Modbus_RxData[Modbus_rxWrite_index] = Modbus_byte;
//		Modbus_rxWrite_index = next_index;
//		last_rx_time = HAL_GetTick();
//		HAL_UART_Receive_IT(&huart3, &Modbus_byte, 1);
//	}
//}

/**
 * @brief Modbus数据帧数据存入缓冲区
 * @param data 1个字节数据
 * @return 无返回
 * @note 缓冲区Modbus_RxData
 */
void Modbus_rx_push(uint8_t data){
  uint16_t next_index = (Modbus_rxWrite_index + 1) % RX_BUF_SIZE;

	if(next_index == Modbus_rxRead_index){
	  Modbus_rxRead_index = (Modbus_rxRead_index + 1) % RX_BUF_SIZE;
	}
	
	Modbus_RxData[Modbus_rxWrite_index] = Modbus_byte;
	Modbus_rxWrite_index = next_index;
	last_rx_time = HAL_GetTick();
}

/**
 * @brief Modbus数据帧解析
 * @param frame 数据指针
 * @param length 数据长度
 * @return 无返回
 * @note 判断数据帧长度、CRC校验、地址匹配、功能项是否支持，再处理发送数据帧给主机
 */
static void RS485_Modbus_Parse(uint8_t *frame, uint16_t len){
  //小于4个字节无效数据帧
	if(len < 4)return;
	
	//CRC校验，不匹配返回
	uint16_t crc_receive = (frame[ len - 2 ] << 8) | frame[ len - 1 ];
	if(CRC16(frame, len-2) != crc_receive)return;
	
	//从机地址不匹配时返回
	if(frame[0] != SLAVE_ADDRESS)return;
	
	//异常帧处理,从机无法处理功能项
	if((frame[1] != 0x03) && (frame[1] != 0x06) && (frame[1] != 0x10)){
		printf("1.frame[1]= %u",frame[1]);
	  BuildExceptionResponse(frame, 0x01);
	}
	
	//匹配功能项
	switch(frame[1]){
		//功能：读保持寄存器
	  case 0x03:
		{
		  uint16_t start_addr = (frame[2] << 8) | frame[3];
			uint16_t reg_count  = (frame[4] << 8) | frame[5];
			if(start_addr > 3){
				//异常帧处理，寄存器地址超范围
				BuildExceptionResponse(frame, 0x02);
				break;
			}
			uint8_t response[5+ reg_count*2];//地址、功能、数据长度、检验值两字节
			response[0] = SLAVE_ADDRESS;
			response[1] = 0x03;
			response[2] = reg_count * 2;
			for(uint8_t i = 0; i < reg_count; i++){
				uint16_t reg_value;
				RS485_Modbus_GetReg(start_addr + i, &reg_value);
			  response[3 + i*2] = (reg_value >> 8) & 0xFF;
				response[4 + i*2] = reg_value & 0xFF;
			}
			uint16_t crc_response = CRC16(response, 3 + reg_count*2);
			response[3 + reg_count * 2] = (crc_response >> 8) & 0xFF;
			response[4 + reg_count * 2] = crc_response & 0xFF;
			Modbus_SendResponse(response, 5+reg_count*2);
			break;
		}
		
		//功能：写单个寄存器
		case 0x06:
		{
		  uint16_t start_addr = ((uint16_t)frame[2] << 8 | frame[3]);
			uint16_t reg_value = ((uint16_t)frame[4] << 8 | frame[5]);
			if(start_addr < 4 && start_addr >20){
				//地址超范围
	      BuildExceptionResponse(frame, 0x02);
				break;
	    }

			if(reg_value > 180){
				//寄存器数值超范围
			  BuildExceptionResponse(frame, 0x03);
				break;
			}
			
			RS485_Modbus_SetReg(start_addr, reg_value);
			Modbus_SendResponse(frame, len);
			break;
		}
		
		//功能：写多个寄存器
		case 0x10:
    {
		  uint16_t start_addr = ((uint16_t)frame[2] << 8 | frame[3]);
			uint16_t reg_count  = ((uint16_t)frame[4] << 8 | frame[5]);
			
			//地址是否越界
			if(start_addr < 4 && start_addr > 20 ){
			  BuildExceptionResponse(frame, 0x02);
				break;
			}
			
			//写数据到寄存器
			for(uint8_t i = 0; i < reg_count; i++){
			  uint16_t reg_value = (frame[7 + i*2] << 8) | frame[8 + i*2];
				RS485_Modbus_SetReg(i + start_addr, reg_value);
			}
			uint8_t response[8];
			response[0] = SLAVE_ADDRESS;
			response[1] = 0x10;
			response[2] = frame[2];
			response[3] = frame[3];
			response[4] = frame[4];
			response[5] = frame[5];
			uint16_t crc_response = CRC16(response, 6);
			response[6] = (crc_response >> 8) & 0xFF;
			response[7] = crc_response & 0xFF;
			Modbus_SendResponse(response, 8);
			break;
		}
		
		default:
			break;
	}
}

/**
 * @brief Modbus定时器
 * @note 缓存区内容存在否、数据帧是否结束、拷贝缓冲区内容到临时变量处理
 */
static void RS485_Modbus_Time(void){
  static uint32_t last_check_time;
	uint32_t now = HAL_GetTick();
	if(now - last_check_time < 1)return;
	last_check_time = now;
	
	//缓存区内容判断
	if(Modbus_rxWrite_index == Modbus_rxRead_index)return;
	
	//数据帧是否结束
	if(last_check_time - last_rx_time < 4)return;
	
	//数据帧长度
	uint16_t frame_len = (RX_BUF_SIZE - Modbus_rxRead_index + Modbus_rxWrite_index) % RX_BUF_SIZE;
	if(frame_len > 0){
	  uint8_t frame[RX_BUF_SIZE];
		for(uint8_t i = 0; i < frame_len; i++){
			uint16_t index = (Modbus_rxRead_index + i) % RX_BUF_SIZE;
		  frame[i] = Modbus_RxData[index];
		}
		Modbus_rxRead_index = Modbus_rxWrite_index;
		RS485_Modbus_Parse(frame, frame_len);
	}
}

/**
 * @brief 异常帧处理函数
 * @param frame 数据指针
 * @param errcode 错误码
 * @note 数据帧异常时调用，并返回错误码
 */
static void BuildExceptionResponse(uint8_t *frame, uint8_t errcode){
	//常见异常码0x01 非法功能码
	//0x02 非法数据地址
	//0x03 非法数据值
	//从机地址 功能码|0x80 异常码 CRC两个字节
  uint8_t except[5];
	except[0] = frame[0];
	except[1] = frame[1] | 0x80;
	except[2] = errcode;
	uint16_t crc_errcode = CRC16(except, 3);
	except[3] = (crc_errcode >> 8) & 0xFF;
	except[4] = crc_errcode & 0xFF;
  Modbus_SendResponse(except, 5);//发送数据
}

/**
 * @brief RS485数据发送函数
 * @param date 数据指针
 * @param len 数据长度
 * @param timeout_ms 超时时间
 * @note 打包发送函数，加了判断处理串口发送超时情况
 */
static void RS485_Send(uint8_t *data, uint16_t len, uint32_t timeout_ms){
  //RS485写状态
  RS485_SetMode(1);
	if(HAL_UART_Transmit(&huart3, data, len, timeout_ms) != HAL_OK){
    //超时回复读状态并返回
	  RS485_SetMode(0);
		return;
	}
	uint32_t start = HAL_GetTick();
	while(__HAL_UART_GET_FLAG(&huart3, UART_FLAG_TC) == RESET){
		//串口数据正在发送或尚未完全发送出去时，TC 标志位是 RESET（0）
		//如果此时串口线断开，不加超时判断会卡死
	  if(HAL_GetTick() - start > timeout_ms){
			//超时处理
		  RS485_SetMode(0);
			return;
		}
	}
	RS485_SetMode(0);
}

//Modbus回复封装函数
static void Modbus_SendResponse(uint8_t *frame, uint8_t len){
  RS485_Send(frame, len, 100);
}
