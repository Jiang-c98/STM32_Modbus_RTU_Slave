/**
 * @file    servo.c
 * @brief   BT模块
 * @author  Cui Jiang
 * @date    2025-08-15
 */

/* 头文件包含 */
#include "User/servo.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "tim.h"
#include "usart.h"
#include "User/share_data.h"

/**
 * @brief 舵机初始化
 * @note GPIO配置、定时器初始化
 */
void Servo_Init(void){
	//1、GPIO、定时器时钟使能
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_TIM4_CLK_ENABLE();        //定时器外设时钟使能
	
	//2、配置PB9为复用推挽输出（TIM4_CH4）
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	//3、配置TIM4：周期20ms，计数频率
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 72-1;                                 //预分频/72 = 1Mhz
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;                 //递增计数
  htim4.Init.Period = 20000-1;                                 //20ms
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;//自动重装载使能
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    printf("HAL_TIM_PWM_Init failed!\r\n");
  }

  //4、配置CH4为PWN模式1
	TIM_OC_InitTypeDef sConfigOC = {0};
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
	//PWM信号与180度舵机的关系：
	//0.5ms----------------0度；1.5/20*20000 =1500
	//1ms -----------------45度；
	//1.5ms----------------90度；
	//2ms -----------------135度；
	//2.5ms ---------------180度；
  sConfigOC.Pulse = 500;  //0度
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    printf("HAL_TIM_PWM_ConfigChannel failed!\r\n");
  }

  //5、启动PWM
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
}

/**
 * @brief 舵机任务
 * @note 获取共享池寄存器地址5的值，执行动作
 */
static void vServo_Task(void *pvParemeters){
  uint8_t last_angle = 0;
	uint8_t target_angle = 0;
	for(;;){
		//获取共享寄存器池角度
	  target_angle = ShareData_GetReg(5);
		
		//限幅
		if(target_angle > 180){
			target_angle = 180;
			ShareData_SetReg(5, 180);
			printf("[servo] Angle limit to 180\r\n");
		}
		
		if(target_angle != last_angle){
		  last_angle = target_angle;
			Servo_SmoothMove(target_angle, 1);
		}
		//平滑过渡
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

/**
 * @brief 舵机任务
 * @note 启动舵机功能
 */
void StartServoTask(void){
	//创建舵机任务
	if(xTaskCreate(vServo_Task, "servo", 128, NULL, 2, NULL) != pdPASS){
	  printf("vServo_Task create failed!\r\n");
	}
}

/**
 * @brief 设置舵机角度
 * @param 舵机角度 
 * @note 控制舵机转动到对应角度，舵机需要50Hz(周期20ms)的方波
         调节高电平占空比转对应角度
				 0.5/20*20000 = 500  对应0度
				 1000  45度
				 1500  90度
				 2000  135度
				 2500  180度
 */
void Servo_SetAngle(uint8_t angle){
  if(angle > 180){
	  angle = 180;
	}
	uint16_t pulse = 500 + (angle * 2000/180);
	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, pulse);
}

/**
 * @brief 平滑过渡
 * @param 舵机角度
 * @param 延时时长
 * @note 设置目标舵机角度，平滑控制舵机转动到对应角度
 */
void Servo_SmoothMove(uint8_t target_angle, uint16_t step_delay_ms){
  static uint8_t current_angle = 0;
	if(target_angle > current_angle){
	  for(int i = current_angle; i <= target_angle; i++){
		  Servo_SetAngle(i);
			HAL_Delay(step_delay_ms);
		}
	}else{
	  for(int i = current_angle; i >= target_angle; i--){
		  Servo_SetAngle(i);
			HAL_Delay(step_delay_ms);
		}
	}
	current_angle = target_angle;
}
