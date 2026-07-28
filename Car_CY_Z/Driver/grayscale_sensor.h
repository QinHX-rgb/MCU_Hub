#ifndef __GRAYSCALE_SENSOR_H
#define __GRAYSCALE_SENSOR_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

// 全局变量
extern uint8_t g_sensor_raw_data;   // 8位原始数据 (0=黑, 1=白)
extern int16_t cx;                  // 重心坐标 (0~70, 254=全黑, 255=丢线)
extern uint8_t Flag;                // 0=全黑停车 1=左转 2=右转 3=全白直行 其他=循线
extern uint8_t Status;              // 当前八路循迹数值 (实时)
extern uint8_t Last_Status;         // 丢线前最后一帧有效数值 (丢线时不更新)

//==================================================================
//  74HC165 引脚 (SysConfig 已配置为 GPIO)
//==================================================================
#define SENSOR_PL_PORT    GPIO_Sensor_PORT        // GPIOB
#define SENSOR_PL_PIN     GPIO_Sensor_PIN_PL_PIN   // PB15 → PL

#define SENSOR_SCK_PORT   GPIO_Sensor_PORT        // GPIOB
#define SENSOR_SCK_PIN    GPIO_Sensor_PIN_SCK_PIN  // PB14 → SCK

#define SENSOR_SDA_PORT   GPIO_Sensor_PORT        // GPIOB
#define SENSOR_SDA_PIN    GPIO_Sensor_PIN_SDA_PIN  // PB13 → SDA (Q7)

//==================================================================
//  引脚操作宏
//==================================================================
#define PL_LOW()    DL_GPIO_clearPins(SENSOR_PL_PORT, SENSOR_PL_PIN)
#define PL_HIGH()   DL_GPIO_setPins(SENSOR_PL_PORT, SENSOR_PL_PIN)
#define SCK_LOW()   DL_GPIO_clearPins(SENSOR_SCK_PORT, SENSOR_SCK_PIN)
#define SCK_HIGH()  DL_GPIO_setPins(SENSOR_SCK_PORT, SENSOR_SCK_PIN)
#define SDA_READ()  (!!DL_GPIO_readPins(SENSOR_SDA_PORT, SENSOR_SDA_PIN))

//==================================================================
//  函数接口
//==================================================================
#define GRAYSCALE_SENSOR_CHANNELS   8

void Grayscale_Sensor_Init(void);
uint8_t Grayscale_Update(void);        // 读取8通道并计算cx

#endif
