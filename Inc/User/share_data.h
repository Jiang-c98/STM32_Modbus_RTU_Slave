#ifndef _SHARE_DATA_H
#define _SHARE_DATA_H

#include <stdint.h>

#define MAX_HOLDING_REGISTERS 20

//光敏、热敏、温度、湿度、继电器开关、舵机角度
extern uint16_t holding_registers[MAX_HOLDING_REGISTERS];

void ShareData_init(void);
void ShareData_SetReg(uint16_t addr, uint16_t value);
uint16_t ShareData_GetReg(uint16_t addr);

#endif
