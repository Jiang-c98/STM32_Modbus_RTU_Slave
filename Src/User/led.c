#include "User\led.h"
#include "User\rs485_modbus.h"

void LED_ALL_On(void)
{
  HAL_GPIO_WritePin(GPIOA, LED1, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LED2, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LED3, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, LED4, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, Buzzer, GPIO_PIN_SET);
}

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
	if(RS485_Modbus_GetReg(2) != 1)return;
  LED_ALL_Off();
  LED_ON(*step_value);		
  *step_value = (*step_value + 1)%3;//0 1 2 3
}


static uint8_t led_is_on;
static uint32_t led_off_time;
//LED1+蜂鸣器工作
void LED_Alert_Request(uint32_t duration_ms)
{
  if(!led_is_on)
	{
		HAL_GPIO_WritePin(GPIOA, LED1, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, Buzzer, GPIO_PIN_SET);
		led_is_on = 1;
		led_off_time = HAL_GetTick() + duration_ms;
	}
	
	if(led_is_on && HAL_GetTick() >= led_off_time){
	  HAL_GPIO_WritePin(GPIOA, LED1, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, Buzzer, GPIO_PIN_RESET);
		led_is_on = 0;
	}
}

//void LED_Alert_Task(void)
//{ 
//  if (led_is_on && HAL_GetTick() >= led_off_time)//判断当前时间是否超过led_off_time
//	{
//		printf("LED_Alert_Task if\r\n");
//	  HAL_GPIO_WritePin(GPIOA, LED1, GPIO_PIN_SET);
//		HAL_GPIO_WritePin(GPIOA, Buzzer, GPIO_PIN_RESET);
//		led_is_on = 0;
//		printf("LED_Alert_Task led_is_on=%d led_off_time=%u",led_is_on,led_off_time);
//	}
//}
