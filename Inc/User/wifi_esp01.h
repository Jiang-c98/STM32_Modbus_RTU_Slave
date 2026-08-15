/**
 * @file    wifi_esp01.h
 * @brief   ESP01 Wi-Fi 模块驱动
 * @author  Cui Jiang
 * @date    2025-08-15
 */

#ifndef __WIFI_ESP01_H
#define __WIFI_ESP01_H

#include <stdint.h>
#include <stdbool.h>

/* 缓冲区长度 */
#define WIFI_RING_BUF_SIZE 256

/* 外部全局变量声明 */
extern uint8_t Wifi_byte;

/* 环形缓冲区结构体 */
typedef struct{
  uint8_t buffer[WIFI_RING_BUF_SIZE];
	volatile uint16_t read;
	volatile uint16_t write;
}ring_buffer_t;

/* WiFi 状态机 */
typedef enum{
  WIFI_IDLE,          //空闲，可发指令
	WIFI_WAITING_OK,    //等待AT指令的OK响应
	WIFI_WAITING_DATA,  //等待TCP数据（+IPD）
	WIFI_WAITING_PROMPT //正在发送TCP数据（等待>提示符）
}WifiState_t;

void Wifi_Init(void);
void wifi_rx_push(uint8_t data);
void StartWifiTask(void);

#endif
