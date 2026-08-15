/**
 * @file    menu_state.h
 * @brief   状态机数据类型定义
 * @author  Cui Jiang
 * @date    2025-06-07
 */

#include <stdint.h>

#ifndef __MENU_STATE_H
#define __MENU_STATE_H
typedef enum{
  STATE_MENU = 0,
	STATE_LIGHT_SENSOR,
	STATE_TEMP_SENSOR,
	STATE_DHT22_SENSOR,
}MenuState_t;

extern MenuState_t currentMenu;//菜单状态机
extern uint8_t mainMenuSelection;//菜单前的标志箭头,0-light,1-temp,2无箭头

#endif
