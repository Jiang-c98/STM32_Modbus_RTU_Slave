#include "User\sensor.h"
#include "User\rs485_modbus.h"

MedianFilter_t light_filter;
MedianFilter_t temp_filter;
//QueueHandle_t xSensorDataQueue;//定义队列句柄
extern ADC_HandleTypeDef hadc1;

static void Sensor_AdcSetCh(uint32_t channel);
static uint16_t Sensor_ReadTemp(void);
static uint16_t Sensor_ReadLight(void);
static uint16_t Sensor_Filtle_Handle(uint16_t new_value, MedianFilter_t *filter);

void Sensor_Handle(void){
	//获取传感器数据，并且中值滤波处理
  uint32_t light_value = Sensor_Filtle_Handle(Sensor_ReadLight(), &light_filter);
	uint32_t temp_value  = Sensor_Filtle_Handle(Sensor_ReadTemp(), &temp_filter);
	
	//保存采集的数据到寄存器
	RS485_Modbus_SetReg(0, light_value);
	RS485_Modbus_SetReg(1, temp_value);
}

//单通道获取-设置通道
static void Sensor_AdcSetCh(uint32_t channel){
	//停止ADC采集
	HAL_ADC_Stop(&hadc1);
  ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = channel;
	sConfig.Rank = ADC_REGULAR_RANK_1;//都用rank1
	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
	//将配置写入adc的配置寄存器
	if(HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
	  Error_Handler();
	}
}	

//热敏adc电压采样  -单通道获取adc
static uint16_t Sensor_ReadTemp(void){
	Sensor_AdcSetCh(ADC_CHANNEL_9);
  uint16_t adc_value;
	HAL_ADC_Start(&hadc1);
	if(HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
	{
	adc_value = HAL_ADC_GetValue(&hadc1);
	}
	HAL_ADC_Start(&hadc1);
	return adc_value;
}

//光敏adc电压采样
static uint16_t Sensor_ReadLight(void){
	Sensor_AdcSetCh(ADC_CHANNEL_8);
	uint16_t adc_value;
  HAL_ADC_Start(&hadc1);
	if(HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
	{
	  adc_value = HAL_ADC_GetValue(&hadc1);
	}
	HAL_ADC_Stop(&hadc1);
	return adc_value;
}

//中值滤波
static uint16_t Sensor_Filtle_Handle(uint16_t new_value, MedianFilter_t *filter){
  uint8_t i, j;
	uint16_t temp[FILTER_SIZE];
	filter->buffer[filter->index] = new_value;
	//环形缓冲区指针，从最近写入的位置开始，逐个往前取数据，遇到数组末尾就绕回到开头
  filter->index = (filter->index + 1) % FILTER_SIZE;
	
	//前五次会filled自增
	if(filter->filled < FILTER_SIZE)
	{
		filter->filled++;
	}
	
	//拷贝数据到临时缓冲区
	uint8_t idx = filter->index;
	for(i = 0; i < filter->filled; i++)
	{
	  temp[i] = filter->buffer[idx];
		idx = (idx +1) % FILTER_SIZE;
	}
	
	//数据排序，从左到右先放置最小的
	for(i = 0; i < filter->filled; i++)
	{
	  for(j = i + 1; j < filter->filled; j++)
		{
		  if(temp[i] > temp[j])//对比大小交换位置
			{
			  uint16_t temp_value = temp[i];
				temp[i] = temp[j];
				temp[j] = temp_value;
			}
		}
	}
	return temp[filter->filled/2];
}

//static void LightSensorTest(void)//根据sensor模块的DO点平判断
//{
//	if(HAL_GPIO_ReadPin(GPIOA, LightSensorDo) == GPIO_PIN_RESET)
//	{
//		for(uint8_t i = 0; i < 2; i++)
//		{
//	    LED_ALL_On();
//		  HAL_Delay(1000);
//			LED_ALL_Off();
//			HAL_Delay(1000);
//		}
//	}
//}

////双通道获取adc，存在串扰不可用当前传感器不适用
//uint16_t light_adc_value, temp_adc_value;
//void ReadBothSensor_adc(uint16_t *light_ptr, uint16_t *temp_ptr)
//{
//  HAL_ADC_Start(&hadc1);//启动adc
//	
//	//等待读取rank1 PB0 光敏
//	if(HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
//	{
//	  *light_ptr = HAL_ADC_GetValue(&hadc1);
//	}else{
//	  *light_ptr = 0;
//	}
//	
//	//等待读取rank2 PB1 热敏
//	if(HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
//	{
//	  *temp_ptr = HAL_ADC_GetValue(&hadc1);
//	}else{
//	  *temp_ptr = 0;
//	}
//	HAL_ADC_Stop(&hadc1);
//}
