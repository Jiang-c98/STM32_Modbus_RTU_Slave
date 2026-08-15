#ifndef __STEPPER_H
#define __STEPPER_H

#include "stdint.h"

void Stepper_Init(void);
void StartStepperTask(void);
void Stepper_Run(uint16_t steps, uint32_t us, uint8_t dir);

#endif
