# STM32 Modbus RTU 从机

基于 STM32F103C8T6 + FreeRTOS 的工业传感器节点，支持本地按键菜单、OLED 显示、光敏/热敏传感器采集，以及 Modbus RTU 远程通信。

## 功能列表

- [x] 光敏/热敏传感器采集（ADC + 中值滤波）
- [x] OLED 显示（U8g2 库 + 页寻址手动刷新）
- [x] 按键菜单（长按/短按，状态机实现）
- [x] FreeRTOS 多任务（传感器、Modbus、UI、LED）
- [x] RS-485 硬件调试（偏置电阻、地环路排查）
- [x] Modbus RTU 从机（功能码 03/06/16，异常码处理）

## 硬件连接

| 模块 | STM32 引脚 | 说明 |
|------|-----------|------|
| OLED | PB6 (SCL), PB7 (SDA) | I2C 接口，0.96 寸 (SSD1306)|
| 光敏传感器 | PB0 (ADC1_IN8) | AO 输出 |
| 热敏传感器 | PB1 (ADC1_IN9) | AO 输出 |
| 按键 | PA0 (EXTI0) | 双边沿触发，按键按下高电平有效 |
| MAX485 | PB10 (TX), PB11 (RX), PA8 (DE/RE) | RS-485 收发 |
| LED | PA1, PA2, PA3, PA5 | 流水灯指示 |

## 软件架构

- **RTOS**：FreeRTOS（任务、队列、信号量、互斥锁）
- **通信协议**：Modbus RTU 从机（03/06/16 功能码 + 异常码）
- **外设驱动**：GPIO、ADC、I2C、UART（中断 + 环形缓冲区）

## 使用说明

### 本地操作
- 短按按键：主菜单切换选项
- 长按按键：进入选中功能（光敏/热敏测试）
- 测试界面长按：返回主菜单

### 远程通信
- Modbus Poll 连接 RS-485，读取保持寄存器地址 0（光敏）和 1（热敏）
- 写入寄存器地址 2（LED 控制），0=熄灭，1=点亮

## 视频演示

【基于STM32与FreeRTOS的工业传感器节点设计与实现】 https://www.bilibili.com/video/BV1fgEs67Ez1/?share_source=copy_web&vd_source=6f2b8b7ded430b6af8e65ca5318243e9

## 依赖

- STM32CubeMX 生成的 HAL 库
- FreeRTOS (CMSIS-RTOS V2)
- U8g2 图形库 (https://github.com/olikraus/u8g2)

## 开发环境

- Keil MDK 5.36
- STM32CubeMX 6.x
- Git

## 作者

Jiang

## License
MIT