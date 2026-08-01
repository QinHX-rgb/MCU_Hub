#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* 控制与显示周期 */
#define CONTROL_PERIOD_MS       5U
#define DISPLAY_UPDATE_FRAMES   40U
/* 计时器 OLED 刷新帧数（= TIMER_REFRESH_MS / CONTROL_PERIOD_MS） */
#define TIMER_REFRESH_MS        100U

/* OLED 始终开启，运行中实时刷新。 */
#define ENABLE_OLED             1
#define ENABLE_UART_DEBUG       1

/* 两档基础速度 PWM（百分比） */
#define BASE_PWM_FAST           38
#define BASE_PWM_SLOW           22
#define CURVE_MIN_PWM           20.0f
#define CURVE_SLOWDOWN          7.0f
#define MIN_PWM                 0
#define MAX_PWM                 40
#define PWM_FILTER_ALPHA        0.18f

/* 慢速档软启动：4 秒内从 8% 线性加速到目标速度 */
#define SLOW_SOFT_START_MS      4000U
#define SLOW_SOFT_START_MIN_PWM 8.0f

/* 定时减速（暂未启用） */
#define SLOW_DECEL_START_MS     24000U
#define SLOW_DECEL_MS           5000U

/* 快速档变速：前 16s 以 40% 行驶，之后降为 25% */
#define FAST_SPEED_SWITCH_MS    13000U
#define FAST_PWM_AFTER          25

/*
 * 灰度循迹参数。连续死区不会在死区边缘产生输出台阶；
 * 位置和误差变化率分别低通，以降低 8 路离散采样造成的蛇形摆动。
 */
#define LINE_CENTER             35
#define LINE_DEAD_ZONE          3.5f
#define LINE_FILTER_ALPHA       0.22f
#define DERIVATIVE_FILTER_ALPHA 0.12f
#define STEER_KP                0.28f
#define STEER_KD                0.35f
#define STEER_LIMIT             8.0f

/*
 * 停车线检测：
 * 起跑时先忽略车底的启停线；连续检测到普通轨迹（1~2 路黑色）
 * 后立即武装。武装后超过 2 路同时为黑即停车。
 */
#define STOP_LINE_BLACK_MIN        3U
#define STOP_LINE_BLACK_MIN_FAST   2U
#define START_LINE_CLEAR_FRAMES    5U
#define FINISH_BRAKE_TIME_MS          150U

/* 当前 8 路灰度模块：黑线输出低电平。 */
#define SENSOR_BLACK_IS_LOW      1

#define LEFT_PWM_TRIM            0
#define RIGHT_PWM_TRIM           0

/* 按钮定义见 SysConfig (GPIO_BUTTON 组):
   S1/PA18: 启动，高电平有效 (PULL_DOWN)
   Key1/PA27: 快速档，低电平有效 (PULL_UP)
   Key2/PB2:  慢速档，低电平有效 (PULL_UP) */

#endif
