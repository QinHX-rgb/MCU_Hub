#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

void Motor_Init(void);
void Motor_ResetControl(void);
void Motor_SetSpeedFast(bool fast);
void Motor_SetDifferential(int8_t left, int8_t right);
void Motor_FollowLine(int16_t position);
void Motor_StartDeceleration(void);

extern int16_t g_dbg_L;
extern int16_t g_dbg_R;

#endif
