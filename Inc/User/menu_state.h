/**
 * @file    menu_state.h
 * @brief   状态机数据类型定义
 * @author  Cui Jiang
 * @date    2025-06-07
 */
 
#ifndef __MENU_STATE_H
#define __MENU_STATE_H
typedef enum{
  STATE_MENU = 0,
	STATE_LIGHT_SENSOR,
	STATE_TEMP_SENSOR,
}MenuState_t;

#endif
