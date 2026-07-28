#ifndef KEY_H
#define KEY_H

#include "ti_msp_dl_config.h"

void    Key_Init(void);    // 重配按键为输入+上拉
uint8_t Key_Read(void);

#endif

