/**
 * @file    DHT22.c
 * @brief   温湿度传感器模块
 * @author  Cui Jiang
 * @date    2025-08-11
 */

/* 头文件包含 */
#include "User/DHT22.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"
#include "usart.h"
#include "User/share_data.h"

/* 私有常量 */
#define DHT22_PIN  GPIO_PIN_5
#define DHT22_PORT GPIOB

/* 私有变量 */
static uint16_t humidity;
static uint16_t temperature;
static uint32_t last_time;
static bool is_initialized;

/* 全局变量 */
uint8_t DHT22_Data[5] = {0};

/* 私有函数声明 */
static void DHT22_TimerInit(void);
static void DHT22_Output_Mode(void);
static void DHT22_Input_Mode(void);
static void DHT22_Read(void);
static bool DHT22_WaitForLevel(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState level, uint32_t timeout_us);
static uint32_t DHT22_MeasureHighTime(GPIO_TypeDef *port, uint16_t pin);

/**
 * @brief DHT22执行任务
 * @note 实现温湿度采集任务
 */
void vDHT22_Task(void *Parameters){
  for(;;){
	  DHT22_Read();     //微秒时序操作
		
		uint16_t humidity,temperture;
	  humidity   = ((uint16_t)DHT22_Data[0] << 8) | DHT22_Data[1];
	  temperture = ((uint16_t)DHT22_Data[2] << 8) | DHT22_Data[3];
	
	  ShareData_SetReg(2, temperture);
	  ShareData_SetReg(3, humidity);
		
		vTaskDelay(pdMS_TO_TICKS(2500));//2.5s采样一次
	}
}

/**
 * @brief DHT22任务
 * @note 实现温湿度采集任务
 */
void StartDHT22Task(void){
  //创建DHT22采集任务
	if(xTaskCreate(vDHT22_Task, "DHT22", 128, NULL, 1, NULL) != pdPASS){
	  printf("vDHT22_Task create failed\r\n");
	}
}

/**
 * @brief 模块初始化
 * @note 初始化定时器用于微秒级延时
 */
void DHT22_Init(void){
  if(!is_initialized){
	  DHT22_TimerInit();
		is_initialized = 1;
	}
}

/**
 * @brief 定时器初始化
 * @note 定时器使能、预分频、自动重装
 */
void DHT22_TimerInit(void){
//  __HAL_RCC_TIM2_CLK_ENABLE();        //定时器外设时钟使能
//	TIM2->PSC = 72 - 1;                 //输入72Mhz，预分频到1/72，由于寄存器从0开始，所以减1
//	TIM2->ARR = 0xFFFFFFFF;             //自动重装载，计数最大值
//	TIM2->CR1 = TIM2->CR1 | TIM_CR1_CEN;//定时器控制寄存器，使能计数
	printf("[DHT22] Using TIM4, CNT=%u\r\n", TIM4->CNT);
}

/**
 * @brief 输出模式设置
 * @note 设置GPIO为推挽输出模式，nopull，高速
 */
static void DHT22_Output_Mode(void){
  GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = DHT22_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct);
}

/**
 * @brief 输入模式设置
 * @note 设置GPIO为输入模式、nopull
 */
static void DHT22_Input_Mode(void){
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = DHT22_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct);
}

//调试使用
//uint32_t high_time_value[40];
//uint8_t bit_index = 0;

/**
 * @brief 读取温湿度传感器数据
 * @note 读取间隔2.5秒，再根据时序要求写读，判断高电平时间长短保存位数据
         校验数据成功后完成解析
 */
static void DHT22_Read(void){
	uint16_t now = HAL_GetTick();
	if(now - last_time < 2500)return;
	last_time = now;
	
	//数据清零
	memset(DHT22_Data, 0, sizeof(DHT22_Data));
	
	//起始信号
	DHT22_Output_Mode();//输出模式
	HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET);//拉低
	delay_us(1000);
	
	//主机释放总线
	HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);//恢复原来状态
	DHT22_Input_Mode();//输入模式
	delay_us(29);

	//等待传感器响应
	if(DHT22_WaitForLevel(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET, 100) == false){
	  printf("DHT22_WaitFor Low timeout\r\n");
		return;
	}
	if(DHT22_WaitForLevel(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET, 100) == false){
	  printf("DHT22_WaitFor High  timeout\r\n");
		return;
	}
	
	//读40位数据
	//i=0时湿度高8位，i=1时湿度低8位
	//i=2时温度高8位，i=3时温度低8位
	//等待低电平结束
	DHT22_WaitForLevel(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET, 80);
	__disable_irq();//关闭中断
	for(uint8_t i = 0; i < 5; i++){
	  for(uint8_t j = 0; j < 8; j++){
			uint32_t high_time = DHT22_MeasureHighTime(DHT22_PORT, DHT22_PIN);
			DHT22_Data[i] = DHT22_Data[i] << 1;
//			high_time_value[bit_index++] = high_time;
			if(high_time > 45 && high_time < 80){
			  DHT22_Data[i] = DHT22_Data[i] | 0x01;
			}else if(high_time > 15 && high_time < 35){
			
			}else{
			//无效值
			}
		}
	}
	__enable_irq();//打开中断
	
//	for(uint8_t j = 0; j < 40; j++){
//	  printf("high_time[%d] = %lu\r\n", j, high_time_value[j]);
//	}
	
//	printf("Data byte: %02X %02X %02X %02X %02X\r\n",DHT22_Data[0], DHT22_Data[1], DHT22_Data[2], DHT22_Data[3], DHT22_Data[4]);
	
	//校验数据
	uint16_t checkvalue = (DHT22_Data[0] + DHT22_Data[1] + DHT22_Data[2] + DHT22_Data[3]) & 0xFF;
	if(DHT22_Data[4] != checkvalue){
	  printf("Checksum error: calc = 0x%02X, recv = 0x%02X\r\n", checkvalue, DHT22_Data[4]);
		return;
	}
	
	humidity = ((uint16_t)DHT22_Data[0] << 8) | DHT22_Data[1];
	temperature = ((uint16_t)DHT22_Data[2] << 8) | DHT22_Data[3];

  printf("humidity = %u.%u %%\r\n", humidity/10, humidity %10);
  printf("temperture = %u.%u C\r\n", temperature/10, temperature %10);
}

// 计算当前时间差值（自动处理 ARR=20000 溢出）
static inline uint32_t DHT22_GetElapsed(uint32_t start){
//内联函数
	uint32_t current = TIM4->CNT;
	if(current >= start){
		return current - start;
	}else{
		return (current + 20000) - start;//溢出后计算
	}
	
}

void delay_us(uint32_t us){
  uint32_t start = TIM4->CNT;
	// 无符号溢出是明确定义的（模 2^32），可以正确处理 CNT 回绕
	while(DHT22_GetElapsed(start) < us)
	{
	//无操作
	}
}

/**
 * @brief DHT22等待电平函数
 * @param GPIO类型
 * @param pin 引脚
 * @param level 高低电平
 * @param timeout_us 微秒级超时时间
 * @note 指定时间范围内等待GPIO接收到指定电平信号
         定时器计数是逐渐增大，sysTick寄存器是逐渐减小
 */
static bool DHT22_WaitForLevel(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState level, uint32_t timeout_us){
  uint32_t start = TIM4->CNT;
	uint32_t elapsed = 0;
	//端口不是level电平
	while(HAL_GPIO_ReadPin(port, pin) != level){
    elapsed = DHT22_GetElapsed(start);
//	  uint32_t elapsed = (TIM4->CNT - start) & 0x00FFFFFF;//防止溢出
		
    //非指定电平持续循环
		if(elapsed > timeout_us){
		  return false;
		}
	}
	return true;
}

/**
 * @brief DHT22等待电平函数
 * @param GPIO类型
 * @param pin 引脚
 * @note 测量微秒级电平变化，先从低电平——>高电平
         记录高电平——>低电平的时间间隔决定'0'或'1'
         TH0 信号“0”高电平时间 min:22  typ:26  max:30 µS
         TH1 信号“1”高电平时间 min:68  typ:70  max:75 µS
 */
static uint32_t DHT22_MeasureHighTime(GPIO_TypeDef *port, uint16_t pin){
  uint32_t start;//TIM4->CNT计数器逐渐增大，若是SysTick寄存器是逐渐减小
	start = TIM4->CNT;
	while(HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET){
	//等待低电平结束
		if(DHT22_GetElapsed(start) > 55)
			return 0xFFFFFFFF;//超过50us返回
	}
	
	start = TIM4->CNT;
	while(HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET){
	  if(DHT22_GetElapsed(start) > 200)
			return 0xFFFFFFFF;//超过200us返回
	}
	
	return DHT22_GetElapsed(start);//差值单位us
}
