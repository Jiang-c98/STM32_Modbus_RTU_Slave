#ifndef __SERVO_H
#define __SERVO_H

#include <stdint.h>

void Servo_Init(void);
void StartServoTask(void);
//直接跳转
void Servo_SetAngle(uint8_t angle);
//平滑过渡
void Servo_SmoothMove(uint8_t target_angle, uint16_t step_delay_ms);


#endif
