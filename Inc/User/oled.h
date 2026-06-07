#ifndef __OLED_H
#define __OLED_H
#include "..\Src\Oled\csrc\u8g2.h"
#include "stdio.h"
#include "User\menu_state.h"

void OLED_Init(void);
void OLED_UpdateDisplay(MenuState_t menustate);

//使用回调函数显示有问题
uint8_t My_U8x8_I2c_HwSend(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t My_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

#endif
