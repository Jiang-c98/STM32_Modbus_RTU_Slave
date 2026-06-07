/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "User\led.h"
#include "User\key.h"
#include "User\sensor.h"
#include "User\oled.h"
#include "User\rs485_modbus.h"
#include "User\menu_state.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
MenuState_t currentMenu;//菜单状态机
uint8_t mainMenuSelection = 2;//菜单前的标志箭头,0-light,1-temp,2无箭头
extern QueueHandle_t xKeyQueue;//按键队列句柄
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
void vSensor_Task(void *Parameters);
void vOLED_Task(void *pvParameters);
void vLED_Task(void *pvParameter);
void vKey_Task(void * Parameters);
void vModbus_Task(void *pvParameters);
void HanldeKeyEvent(KeyEvent_t *key);
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
	OLED_Init();
	Modbus_Init();
	Key_Init();
	
	//创建Sensor任务
	if(xTaskCreate(vSensor_Task, "Sensor", 256, NULL, 2, NULL) != pdPASS){
    printf("vSensor_Task create failed\r\n");
	}
	//创建LED任务
  if(xTaskCreate(vLED_Task, "led", 128, NULL, 2, NULL) != pdPASS){
	  printf("vLED_Task create failed\r\n");
	}

	//创建OLED任务
	if(xTaskCreate(vOLED_Task, "Oled", 256, NULL, 2, NULL) != pdPASS){
	  printf("vOLED_Task create failed\r\n");
	}
	
	//创建Key任务
	if(xTaskCreate(vKey_Task, "key", 256, NULL, 1, NULL) != pdPASS){
		printf("Key Task create failed!");
	}
	
	//创建Modbus任务
	xTaskCreate(vModbus_Task, "modbus", 512, NULL, 1, NULL);
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void vSensor_Task(void *Parameters){
  TickType_t xLastWakeTime = xTaskGetTickCount();
	const TickType_t xFrequency = pdMS_TO_TICKS(200);
	for(;;){
		Sensor_Handle();
		
		//绝对延时200ms，会考虑到其他任务执行消耗的时间并计算在内
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		//vTaskDelay(pdMS_TO_TICKS(500));//相对延时500ms
	}
}

//按键任务
void vKey_Task(void * Parameters){
	for(;;){
		Key_Scan();
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

//OLED显示任务
void vOLED_Task(void *pvParameters){	
	//调试阶段,任务入口打印栈余量
	//UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
  KeyEvent_t key_receive;
	for(;;){
		OLED_UpdateDisplay(currentMenu);
    if(xQueueReceive(xKeyQueue, &key_receive, pdMS_TO_TICKS(200)) == pdTRUE){
		  //状态机处理
			HanldeKeyEvent(&key_receive);
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

//LED任务
void vLED_Task(void *pvParameter){
	printf("vLED_Task start\r\n");
	TickType_t lastWakeTime = xTaskGetTickCount();
//  uint8_t step = 0;
	for(;;){
//    LED_River(&step);//流水灯
		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

//Modbus任务
void vModbus_Task(void *pvParameters){
  for(;;){
	  RS485_Modbus_Time();
    vTaskDelay(pdMS_TO_TICKS(10));
	}
}

//按键菜单状态机
void HanldeKeyEvent(KeyEvent_t *key){
  switch(currentMenu){
	  case STATE_MENU:
			if(key->key_event == 0){
        mainMenuSelection = (mainMenuSelection == 0)? 1:0;
			}else if(key->key_event == 1){
			  currentMenu = (mainMenuSelection == 0)?STATE_LIGHT_SENSOR:STATE_TEMP_SENSOR;
			}
			break;
			
		case STATE_LIGHT_SENSOR:
		case STATE_TEMP_SENSOR:
			if(key->key_event == 1){
			  currentMenu = STATE_MENU;
			}
			break;
	}
}

//堆栈溢出测试函数
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("Stack overflow in %s\r\n", pcTaskName);
    while (1);
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
