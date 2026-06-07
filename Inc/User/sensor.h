#ifndef __SENSOR_H
#define __SENSOR_H
#include "main.h"
#include "User\led.h"
#include "stdio.h"


#define LightSensorDo GPIO_PIN_7 //光敏DO口
#define FILTER_SIZE 5 //滤波器长度
	
typedef struct{
  uint16_t buffer[FILTER_SIZE];
	uint8_t index;
	uint8_t filled;
}MedianFilter_t;//中值滤波器

void Sensor_Handle(void);

#endif
