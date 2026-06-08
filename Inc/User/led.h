/**
 * @file    led.h
 * @brief   LED控制模块
 * @author  Cui Jiang
 * @version 1.0
 * @date    2025-06-07
 */
 
#ifndef __LED_H
#define __LED_H
#include "stdint.h"

#define LED1 GPIO_PIN_1//PA1
#define LED2 GPIO_PIN_2//PA2
#define LED3 GPIO_PIN_3//PA3
#define LED4 GPIO_PIN_5//PA5
#define Buzzer GPIO_PIN_6//PA6

void LED_ALL_On(void);
void LED_ALL_Off(void);
void LED_ON(uint8_t led_step);
void LED_River(uint8_t *step_value);
#endif
