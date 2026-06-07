#ifndef __KEY_H
#define __KEY_H
#include "stdint.h"


//GPIOA
#define Key1 GPIO_PIN_0
#define Key2 GPIO_PIN_13

//按键状态
#define KEY_NONE 0
#define KEY1_PRESSED 1
#define KEY2_PRESSED 2

//消抖时间
#define KEY_DEBOUNCE_TIME 20

typedef struct{
  uint8_t key_code;//按键码
	uint8_t key_event;//事件0=短按，1=长按
}KeyEvent_t;

void Key_Init(void);
void Key_Scan(void);
uint8_t Key_event_status(void);
void Key_event_change(uint8_t set_value);
#endif
