/**
 * @file    stepper.c
 * @brief   步进电机模块
 * @author  Cui Jiang
 * @date    2025-08-15
 */

/* 头文件包含 */
#include "User/stepper.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "usart.h"
#include "stm32f1xx_hal.h"
#include "User/DHT22.h"
#include "User/share_data.h"

/* 私有宏定义 */
#define STEPPER_DIR_CW   0   // 顺时针（正转）
#define STEPPER_DIR_CCW  1   // 逆时针（反转） 

/* 私有常量 */
static const uint8_t step_seq[8] = {
	// 四相八拍：A → AB → B → BC → C → CD → D → DA
	// 物理磁性层面不同通电状态产生的磁力拉动转子转动
	// 不同通电状态 → 磁力方向变化 → 转子跟着转
  // 引脚映射：A=PB15, B=PB14, C=PB13, D=PA15
 	  0x01,  // 0001 (A)
    0x03,  // 0011 (AB)
    0x02,  // 0010 (B)
    0x06,  // 0110 (BC)
    0x04,  // 0100 (C)
    0x0C,  // 1100 (CD)
    0x08,  // 1000 (D)
    0x09   // 1001 (DA)
};

static void Stepper_SetStep(uint8_t step){
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, (GPIO_PinState)(step & 0x01));
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, (GPIO_PinState)(step & 0x02));
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, (GPIO_PinState)(step & 0x04));
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, (GPIO_PinState)(step & 0x08));
}

void Stepper_Init(void){
  //1.禁用JIAG,保留SWD（PA13/PA14）
	__HAL_RCC_AFIO_CLK_ENABLE();
	__HAL_AFIO_REMAP_SWJ_NOJTAG();
	
	//2.初始化GPIO PB15 PB14 PB13
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	//PA15
	GPIO_InitStruct.Pin = GPIO_PIN_15;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	//初始状态：全部拉低
	Stepper_SetStep(0);
}

/**
* @brief 步进电机任务
 * @note 步进电机任务
 */
static void vStepper_Task(void *pvParameters){
	uint16_t cmd = 0;
	uint16_t last_cmd = 0xFFFF;  // 记录上一次指令，避免重复执行
	
	for(;;) {
		// 1. 读取控制寄存器
		cmd = ShareData_GetReg(6);  // 假设地址 6
		
		// 2. 检查是否有新指令（避免重复执行相同指令）
		if(cmd != last_cmd) {
			last_cmd = cmd;
			
			switch(cmd) {
				case 0:  // 停止
						Stepper_SetStep(0);
						printf("[STEPPER] Stop\r\n");
						break;
						
				case 1:  // 正转一圈
						printf("[STEPPER] Forward 1 circle\r\n");
						Stepper_Run(4096, 1200, STEPPER_DIR_CW);
						Stepper_SetStep(0);
						break;
						
				case 2:  // 反转一圈
						printf("[STEPPER] Reverse 1 circle\r\n");
						Stepper_Run(4096, 1200, STEPPER_DIR_CCW);
						Stepper_SetStep(0);
						break;
						
				case 3:  // 正转两圈
						printf("[STEPPER] Forward 2 circles\r\n");
						Stepper_Run(8192, 1000, STEPPER_DIR_CW);
						Stepper_SetStep(0);
						break;
						
				case 4:  // 反转两圈
						printf("[STEPPER] Reverse 2 circles\r\n");
						Stepper_Run(8192, 1000, STEPPER_DIR_CCW);
						Stepper_SetStep(0);
						break;
						
				default:
						printf("[STEPPER] Unknown cmd: %d\r\n", cmd);
						break;
			}
		}
		
		// 3. 每 10ms 检查一次，不影响其他任务
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

/**
 * @brief 步进电机任务
 * @note 步进电机任务
 */
void StartStepperTask(void){
	//创建WIFI主任务
	int ret;
	ret = xTaskCreate(vStepper_Task, "Stepper", 128, NULL, 2, NULL);
	if(ret != pdPASS){
	  printf("vStepper_Task create failed!\r\n");
		printf("ret = %d\r\n", ret);
	}
}

/**
 * @brief 步进电机驱动函数
 * @param steps步数 8步转子转一圈，转子64圈外部转动轴转1/8圈
 * @param us延时 建议 1200~2000，低于1000会丢步
 * @param dir 转动方向，相序表从0~7为正向，从7~0为反向
 * @note 步进电机任务，低优先级，会被舵机抢占
 */
static void Stepper_Run(uint16_t steps, uint32_t us, uint8_t dir){
//正常情况：
//通电 A 相 → 转子被吸到 A 位置（需要时间）→ 切换 B 相 → 转子从 A 被拉到 B
	//延时时间太多，会出现抖动或者丢步（转不到指定角度）情况	
  for(uint16_t i = 0; i < steps; i++){
		uint8_t idx;
		if(dir == STEPPER_DIR_CW){
		  idx = i % 8;
		}else{
		  idx = (8 - (i % 8)) % 8;//从最后一个数组倒数
		}
	  Stepper_SetStep(step_seq[idx]);
		delay_us(us);//控制转速
	}
	//停止时所有引脚拉低，放置发热
	Stepper_SetStep(0);
}
