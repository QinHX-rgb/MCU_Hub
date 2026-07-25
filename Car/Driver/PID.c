#include "PID.h"

void PID_Init(PID_TypeDef *pid, float Kp, float Ki, float Kd, float max, float min)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->integral = 0.0f;
    pid->last_error = 0.0f;
    pid->output_max = max;
    pid->output_min = min;
}

float PID_Update(PID_TypeDef *pid, float error)
{
    // 比例项
    float P = pid->Kp * error;
    
    // 积分项（带限幅）
    pid->integral += error;
    if (pid->integral > 100.0f) pid->integral = 100.0f;
    else if (pid->integral < -100.0f) pid->integral = -100.0f;
    float I = pid->Ki * pid->integral;
    
    // 微分项
    float derivative = error - pid->last_error;
    float D = pid->Kd * derivative;
    pid->last_error = error;
    
    // 总输出
    float output = P + I + D;
    
    // 输出限幅
    if (output > pid->output_max) output = pid->output_max;
    else if (output < pid->output_min) output = pid->output_min;
    
    return output;
}

void PID_Reset(PID_TypeDef *pid)
{
    pid->integral = 0.0f;
    pid->last_error = 0.0f;
}