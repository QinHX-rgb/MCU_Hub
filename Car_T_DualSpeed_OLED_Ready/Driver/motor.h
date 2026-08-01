#ifndef MOTOR_H
#define MOTOR_H

#include "ti_msp_dl_config.h"

void Motor_On(void);
void Motor_Off(void);
void Motor_Brake(void);
void Set_Speed(uint8_t side, int8_t duty);


#endif
