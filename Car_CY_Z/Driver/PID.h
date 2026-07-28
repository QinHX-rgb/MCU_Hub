#ifndef PID_H
#define PID_H

#include <stdint.h>

typedef struct {
    float Kp;
    float Ki;
    float Kd;
    float integral;
    float last_error;
    float output_max;
    float output_min;
} PID_TypeDef;

void PID_Init(PID_TypeDef *pid, float Kp, float Ki, float Kd, float max, float min);
float PID_Update(PID_TypeDef *pid, float error);
void PID_Reset(PID_TypeDef *pid);

#endif