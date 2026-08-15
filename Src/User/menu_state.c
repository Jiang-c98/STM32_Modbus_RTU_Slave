/**
 * @file    menu_state.c
 * @brief   菜单状态机
 * @author  Cui Jiang
 * @date    2025-08-11
 */

#include "User/menu_state.h"

MenuState_t currentMenu;//菜单状态机
uint8_t mainMenuSelection = 0;//菜单前的标志箭头,0-light,1-temp,2-DHT22
