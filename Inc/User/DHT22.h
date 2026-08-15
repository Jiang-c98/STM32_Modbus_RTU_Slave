#ifndef __DHT22_H
#define __DHT22_H

#include "stdint.h"

void DHT22_Init(void);
void StartDHT22Task(void);
void delay_us(uint32_t us);

#endif
