#ifndef MOTOR_H
#define MOTOR_H

#include "ti_msp_dl_config.h"

/*==================================================================
 *  电机控制参数
 *==================================================================*/

/*—— 传感器 ——*/
#define LINE_CENTER   35      // 传感器重心理想位置（0~70 中點）

/*—— 死区与滤波 ——*/
#define DEAD_ZONE     2       // 误差死区（±2），防止小误差抖动
#define FILTER_ALPHA  0.65f   // 输出低通滤波系数 (0~1，越大越灵敏)

/*—— PWM 限幅 ——*/
#define BASE_PWM      26      // 直行基础 PWM 占空比 (%)
#define MAX_PWM       35      // 最大 PWM 占空比
#define MIN_PWM       10      // 最小 PWM（避免转弯时电机停转）

void Motor_On(void);
void Motor_Off(void);
void Set_Speed(uint8_t side, int8_t duty);


#endif
