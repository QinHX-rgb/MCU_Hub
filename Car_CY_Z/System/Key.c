#include "Key.h"

/* 按键扫描进程
 * Key1=PB14, Key2=PB16 (SysConfig 默认配为输出，Key_Init 重配为输入+上拉)
 * 按下为低电平，返回 1/2/3，无按键返回 0 */

void Key_Init(void)
{
    /* 先关输出驱动，再配为下拉输入 */
    DL_GPIO_disableOutput(GPIO_BUTTON_PIN_Key1_PORT, GPIO_BUTTON_PIN_Key1_PIN);
    DL_GPIO_initDigitalInputFeatures(GPIO_BUTTON_PIN_Key1_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_disableOutput(GPIO_BUTTON_PIN_Key2_PORT, GPIO_BUTTON_PIN_Key2_PIN);
    DL_GPIO_initDigitalInputFeatures(GPIO_BUTTON_PIN_Key2_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
}

uint8_t Key_Read(void)
{
    unsigned char temp = 0;
    if (DL_GPIO_readPins(GPIO_BUTTON_PIN_Key1_PORT, GPIO_BUTTON_PIN_Key1_PIN)) temp = 1;
    if (DL_GPIO_readPins(GPIO_BUTTON_PIN_Key2_PORT, GPIO_BUTTON_PIN_Key2_PIN)) temp = 2;
    if (DL_GPIO_readPins(GPIO_BUTTON_PIN_Key3_PORT, GPIO_BUTTON_PIN_Key3_PIN)) temp = 3;
    return temp;
}
