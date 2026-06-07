#ifndef __RS485_MODBUS_H
#define __RS485_MODBUS_H
#include "FreeRTOS.h"
#include "semphr.h"
#include "projdefs.h"
#include "usart.h"


#define RX_BUF_SIZE 256
#define SLAVE_ADDRESS 0x01
void Modbus_Init(void);
void RS485_Modbus_Time(void);
void RS485_Modbus_SetReg(uint16_t addr, uint16_t value);
uint16_t RS485_Modbus_GetReg(uint16_t addr);
#endif
