/**
 * @file    bluetooth_jdy31.h
 * @brief   BT 模块驱动
 * @author  Cui Jiang
 * @date    2025-08-15
 */

#ifndef __BLUETOOTH_JDY31_H
#define __BLUETOOTH_JDY31_H

#include <stdint.h>

extern uint8_t BT_byte;

void bt_rx_push(uint8_t data);
uint8_t bt_rx_pop(uint8_t *data);
void BT_Init(void);
void StartBTTask(void);
#endif
