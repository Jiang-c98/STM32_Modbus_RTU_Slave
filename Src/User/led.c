/**
 * @file    led.c
 * @brief   LED控制模块
 * @author  Cui Jiang
 * @date    2025-06-07
 */

/* 头文件包含 */
#include "User/led.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "User/share_data.h"
#include "usart.h"

/* 私有变量 */
static uint8_t led_is_on;
static uint32_t led_off_time;

/* 私有函数声明 */
static void LED_Alert_Request(uint32_t duration_ms);
static void LED_Alert_Task(void);

/**
 * @brief LED任务
 * @note  LED任务
 */
void vLED_Task(void *pvParameter){
	printf("vLED_Task start\r\n");
	TickType_t lastWakeTime = xTaskGetTickCount();
//  uint8_t step = 0;
	for(;;){
//    LED_River(&step);//流水灯
		uint16_t temp_value = ShareData_GetReg(1);
		uint8_t LED_B = ShareData_GetReg(4);
		if(LED_B == 1){
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
		}else if(LED_B == 0){
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
		}
		
		if(temp_value == 0)continue;
		
		if(temp_value <= 1600){
		  LED_Alert_Request(1000);
		}
		LED_Alert_Task();
		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

/**
 * @brief LED任务
 * @note  启动LED任务
 */
void StartLedTask(void){
	//创建LED任务
  if(xTaskCreate(vLED_Task, "led", 128, NULL, 2, NULL) != pdPASS){
	  printf("vLED_Task create failed\r\n");
	}
}

//LED全亮
void LED_ALL_On(void)
{
  HAL_GPIO_WritePin(GPIOA, LED1, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LED2, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LED3, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, LED4, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, Buzzer, GPIO_PIN_SET);
}

//亮灯
void LED_ON(uint8_t led_step)
{
	switch(led_step)
	{
	  case 0: HAL_GPIO_WritePin(GPIOA, LED1, GPIO_PIN_RESET);break;
		
		case 1: HAL_GPIO_WritePin(GPIOA, LED2, GPIO_PIN_RESET);break;
		
		case 2: HAL_GPIO_WritePin(GPIOA, LED3, GPIO_PIN_RESET);break;
		
//		case 3: HAL_GPIO_WritePin(GPIOA, LED4, GPIO_PIN_SET);break;
		
		default:break;
	}
}

//灯全灭
void LED_ALL_Off(void)
{
  HAL_GPIO_WritePin(GPIOA, LED1, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, LED2, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, LED3, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, LED4, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, Buzzer, GPIO_PIN_RESET);
}

//流水灯
void LED_River(uint8_t *step_value){
//	printf("Modbus_GetRegister(2) = %u",Modbus_GetRegister(2));
	if(ShareData_GetReg(2) != 1)return;
  LED_ALL_Off();
  LED_ON(*step_value);		
  *step_value = (*step_value + 1)%3;//0 1 2 3
}

//LED1+蜂鸣器工作
static void LED_Alert_Request(uint32_t duration_ms)
{
	if(led_is_on == 0)
	{
		HAL_GPIO_WritePin(GPIOA, LED1, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, Buzzer, GPIO_PIN_SET);
		led_is_on = 1;
		led_off_time = HAL_GetTick() + duration_ms;
	}
}

static void LED_Alert_Task(void)
{ 
  if (led_is_on && HAL_GetTick() >= led_off_time)//判断当前时间是否超过led_off_time
	{
		printf("LED_Alert_Task if\r\n");
	  HAL_GPIO_WritePin(GPIOA, LED1, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, Buzzer, GPIO_PIN_RESET);
		led_is_on = 0;
		printf("LED_Alert_Task led_is_on=%d led_off_time=%u",led_is_on,led_off_time);
	}
}
