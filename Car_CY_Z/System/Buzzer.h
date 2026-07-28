#ifndef BUZZER_H
#define BUZZER_H

#include "ti_msp_dl_config.h"

/* 蜂鸣器 — PA12，丢线直行时短鸣一声 */

void Buzzer_Init(void);
void Buzzer_Beep(void);     // 短鸣一声 (~100ms)

#endif
