#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "ti_msp_dl_config.h"

/*==================================================================
 *  行驶状态控制函数
 *==================================================================*/

/* 初始化所有控制用的 PID 实例（循迹/直行/转弯） */
void Motor_Control_Init(void);

/* 循线行驶：循迹环(PID steering) + 速度环(base PWM) */
void Control_Line_Track(void);

/* 直行：速度环 + 角度环（CY-Z 陀螺仪航向保持） */
void Control_Straight(void);
void Control_Straight_Reset(void);  // 复位首次进入标志（重新归零角度）
float   Motor_Control_GetStraightRef(void);   // 获取当前直行参考角度
uint8_t Motor_Control_GetLostCount(void);     // 获取丢线直行次数

/* 直角转弯: dir>0 右转, dir<0 左转，转到 Flag==2 为止 */
void Control_Corner(int8_t dir);

#endif
