#include "User/share_data.h"
#include "FreeRTOS.h"
#include "semphr.h"

//寄存器池定义
//前两个分别是光敏、热敏、温度、湿度、后三个写寄存器
static uint16_t holding_registers[MAX_HOLDING_REGISTERS] = {0};
//寄存器需要加锁，防止读取时读到部分旧值和新值
static SemaphoreHandle_t xRegMutex = NULL;//定义互斥锁

void ShareData_init(void){
	xRegMutex = xSemaphoreCreateMutex();
	if(xRegMutex == NULL){
	//预留
	}
}

/**
 * @brief 设置寄存器
 * @param addr 寄存器地址
 * @param value 数据
 * @note 寄存器有长度，在写入数据前加互斥锁，写完在释放互斥锁
 */
void ShareData_SetReg(uint16_t addr, uint16_t value){
	xSemaphoreTake(xRegMutex, portMAX_DELAY);
  if(addr > 10)return;
	
	holding_registers[addr] = value;
	xSemaphoreGive(xRegMutex);
}

//获取寄存器内容
uint16_t ShareData_GetReg(uint16_t addr){
	uint16_t value = 0;
  if(addr > 10)return 0;

	xSemaphoreTake(xRegMutex, portMAX_DELAY);//加锁
	value = holding_registers[addr];
	xSemaphoreGive(xRegMutex);
	return value;
}
