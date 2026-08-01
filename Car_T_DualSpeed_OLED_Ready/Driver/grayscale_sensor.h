#ifndef GRAYSCALE_SENSOR_H
#define GRAYSCALE_SENSOR_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

#define GRAYSCALE_SENSOR_CHANNELS 8

extern uint8_t g_sensor_raw_data;
extern int16_t cx;

#define SENSOR_PL_PORT   GPIO_Sensor_PORT
#define SENSOR_PL_PIN    GPIO_Sensor_PIN_PL_PIN
#define SENSOR_SCK_PORT  GPIO_Sensor_PORT
#define SENSOR_SCK_PIN   GPIO_Sensor_PIN_SCK_PIN
#define SENSOR_SDA_PORT  GPIO_Sensor_PORT
#define SENSOR_SDA_PIN   GPIO_Sensor_PIN_SDA_PIN

#define PL_LOW()   DL_GPIO_clearPins(SENSOR_PL_PORT, SENSOR_PL_PIN)
#define PL_HIGH()  DL_GPIO_setPins(SENSOR_PL_PORT, SENSOR_PL_PIN)
#define SCK_LOW()  DL_GPIO_clearPins(SENSOR_SCK_PORT, SENSOR_SCK_PIN)
#define SCK_HIGH() DL_GPIO_setPins(SENSOR_SCK_PORT, SENSOR_SCK_PIN)
#define SDA_READ() (!!DL_GPIO_readPins(SENSOR_SDA_PORT, SENSOR_SDA_PIN))

void Grayscale_Sensor_Init(void);

/* 读取 8 路灰度、更新 cx，并返回本帧检测到黑线的通道数。 */
uint8_t Grayscale_Update(void);

#endif
