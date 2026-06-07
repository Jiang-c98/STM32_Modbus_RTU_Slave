#include "FreeRTOS.h"
//#include "queue.h"
#include "semphr.h"
#include "User\key.h"
#include "usart.h"//打印使用
#include "queue.h"

KeyEvent_t key;//按键
QueueHandle_t xKeySemaphore;//定义按键队列句柄-信号量
QueueHandle_t xKeyQueue;

void Key_Init(void){
	//中断发信号，任务收信号的效果，用二值信号量
  xKeySemaphore = xSemaphoreCreateBinary();
	xKeyQueue = xQueueCreate(1, sizeof(KeyEvent_t));//创建按键队列
	key.key_event = 2;//初始值2
	if(xKeySemaphore == NULL){
		//创建失败，返回NULL
		//堆空间不足是可能的原因
	  printf("xKeySemaphore == NULL");
	}
}

//外部中断PA0
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_5);//测试
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;	
	//如果队列操作有触发更高优先级的任务，第二个参数置为pdTURE
	//中断触发给锁-二值量
	xSemaphoreGiveFromISR(xKeySemaphore, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

//按键扫描RTOS
void Key_Scan(void){
	static uint32_t press_start_time = 0;
  static uint8_t press_state = 0;
	if(xSemaphoreTake(xKeySemaphore, portMAX_DELAY) == pdTRUE){//获取信号量
		if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0) == GPIO_PIN_SET){//按键按下时进入
      if(press_start_time == 0){
			  press_state = 1;
			  press_start_time = xTaskGetTickCount();
			}
		}else{
		 	if(press_state == 1){
			  uint32_t duration =xTaskGetTickCount() - press_start_time;
				if(duration > 1000){
				  key.key_event = 1;//长按
				}else{
				  key.key_event = 0;
				}
        xQueueSend(xKeyQueue, &key, 0);
			}
		  press_start_time = 0;//按键标志清零
			press_start_time = 0;
		}
	}
}

//获取开关值
uint8_t Key_event_status(void){
  return key.key_event;
}

//写入开关值
void Key_event_change(uint8_t set_value){
  key.key_event = set_value;
}

//逻辑代码，逻辑可复用先注释掉
//uint8_t Key_Scan(void)
//{
//	static uint8_t last_key_state = KEY_NONE;
//	static uint32_t last_tick = 0;
//	uint8_t current_key = KEY_NONE;
//	if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
//	{
//	  current_key = KEY1_PRESSED;
//	}else if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET){
//    current_key = KEY2_PRESSED;
//	}
//  if(current_key != KEY_NONE){
//	  if(last_tick == 0){
//		  last_tick = HAL_GetTick();
//		}else if((HAL_GetTick() - last_tick) > KEY_DEBOUNCE_TIME){
//		  last_key_state = current_key;
//		}
//	}else{
//	    last_tick = 0;
//	}
//	
//	if(last_key_state != KEY_NONE){
//	  return last_key_state;
//	}
//	return KEY_NONE;
//		
//}
