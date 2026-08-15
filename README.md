# STM32 + FreeRTOS 工业传感器节点

## 项目简介
基于 STM32F103C8T6 + FreeRTOS 的多功能嵌入式传感器节点，集成本地显示、工业总线通信与双通道无线控制。

**支持功能：**
- 传感器采集：DHT22 温湿度、光敏/热敏 ADC
- 本地显示：SSD1306 OLED，按键切换菜单
- 工业通信：RS485 Modbus RTU 从机（03/06/16 功能码）
- 无线控制：WiFi（ESP-01S TCP Server）/蓝牙（JDY-31 透传）
- 执行器控制：舵机 SG90（PWM 角度控制）、步进电机 28BYJ48（四相八拍位置控制）

## 硬件资源
| 外设 | 型号/引脚 |
|------|-----------|
| MCU | STM32F103C8T6 |
| 温湿度 | DHT22（PB5） |
| 光敏/热敏 | ADC（PA0、PB0、PB1） |
| 显示 | SSD1306 OLED（I2C：PB6、PB7） |
| 按键 | PA0（双边沿中断） |
| RS485 | MAX485（USART3：PB10/PB11） |
| WiFi/蓝牙 | ESP-01S（USART2：PA2/PA3） / 蓝牙JDY-31（透传） |
| 舵机 | SG90（TIM4_CH4：PB9） |
| 步进电机 | 28BYJ48（PB15/PB14/PB13/PA15） |

## 软件架构
- **OS**：FreeRTOS（任务管理、队列、互斥锁）
- **数据中枢**：共享寄存器池（所有任务通过寄存器池交互）
- **任务划分**：
- 
| 任务 | 优先级 | 周期 | 说明 |
|------|--------|------|------|
| WiFi 任务 | 3 | 10ms | TCP Server，远程控制 |
| 蓝牙任务 | 2 | 10ms | 透传数据解析 |
| 舵机任务 | 2 | 10ms | PWM 角度控制 |
| 步进电机任务 | 1 | 10ms | 位置控制 |
| 传感器任务 | 2 | 100ms | ADC/DHT22 采集 |
| OLED 任务 | 1 | 200ms | 屏幕刷新 |
| 按键任务 | 2 | 20ms | 长短按检测 |
| WiFi 看门狗 | 1 | 5s | 异常复位 |

### 本地操作
- 短按按键：主菜单切换选项
- 长按按键：进入选中功能（光敏/热敏测试）
- 测试界面长按：返回主菜单

## 通信协议
- **Modbus RTU**：自主实现从机协议栈，支持功能码 03（读寄存器）、06（写单寄存器）、16（写多寄存器），CRC16 校验。
- **WiFi 控制**：ESP-01S AT 指令状态机 + TCP Server，手机通过 TCP 客户端发送指令。
- **蓝牙控制**：JDY-31 透传模式，手机通过蓝牙串口 APP 发送指令。

## 代码结构
Core/Src/User/
├── share_data.c # 共享寄存器池
├── wifi_esp01.c # ESP-01S WiFi 驱动
├── bluetooth_jdy31.c # JDY-31 蓝牙驱动
├── rs485_modbus.c # Modbus RTU 协议栈
├── dht22.c # DHT22 传感器驱动
├── sensor.c # ADC 采集
├── oled.c # SSD1306 显示
├── key.c # 按键检测
├── servo.c # 舵机 PWM 控制
├── stepper.c # 步进电机控制
└── menu_state.c # 菜单状态机

## 视频演示

【项目描述：设计并实现了一个面向工业场景的嵌入式传感器控制节点，具备本地数据采集、人机交互、多路无线通信调试、执行机构驱动与远程监控功能。】 https://www.bilibili.com/video/BV1L6b26YEkZ/?share_source=copy_web&vd_source=6f2b8b7ded430b6af8e65ca5318243e9
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
