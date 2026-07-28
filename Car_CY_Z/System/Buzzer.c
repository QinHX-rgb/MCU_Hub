#include "Buzzer.h"
#include "delay.h"

/* 蜂鸣器引脚: PA12 (IOMUX_PINCM34) — 高电平触发 */
#define BUZZER_PORT   GPIOA
#define BUZZER_PIN    DL_GPIO_PIN_12
#define BUZZER_IOMUX  IOMUX_PINCM34

void Buzzer_Init(void)
{
    DL_GPIO_initDigitalOutput(BUZZER_IOMUX);
    DL_GPIO_clearPins(BUZZER_PORT, BUZZER_PIN);  // 低电平 = 关闭
}

void Buzzer_Beep(void)
{
    DL_GPIO_setPins(BUZZER_PORT, BUZZER_PIN);    // 高电平 = 鸣响
    delay_ms(200);
    DL_GPIO_clearPins(BUZZER_PORT, BUZZER_PIN);  // 低电平 = 关闭
}
