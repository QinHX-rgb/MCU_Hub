/*
 * balance.h - K230 ball position reader + PID servo controller
 *
 * Receives $BALL,帧号,有效标志,位置mm,目标mm,置信度,帧间隔ms#\n
 * from K230 via UART1 (PB7=RX), runs PID, drives servo on PA8.
 */

#ifndef BALANCE_H
#define BALANCE_H

#include <stdint.h>

/* ---- Parsed K230 protocol frame ---- */
typedef struct {
    int32_t frame_num;      /* 序号 */
    uint8_t valid;          /* 有效标志: 1=跟踪中, 0=标定/丢失 */
    float   position_mm;    /* 位置 (mm) */
    float   target_mm;      /* 目标 (mm)，固定为 0 */
    float   confidence;     /* 置信度 0.0~1.0 */
    int32_t dt_ms;          /* 帧间隔 (ms) */
} ball_data_t;

/* ---- 舵机平衡角度 ---- */
#define BALANCE_ANGLE            55      /* 平衡位置舵机角度（管水平时的角度，可调）*/
#define SERVO_HARD_MIN           10      /* 硬限位（与 servo.h 保持一致）*/
#define SERVO_HARD_MAX           140

/* ---- Center 模式 PID（0mm 停稳）---- */
#define CENTER_KP               1.0f
#define CENTER_KI               0.025f
#define CENTER_KD               18.0f

/* ---- 序列 CATCH PID（-50mm 接球）---- */
#define SEQ_KP                  1.20f
#define SEQ_KI                  0.02f
#define SEQ_KD                  18.0f

#define BALANCE_OUT_MAX         40.0f   /* 最大正向倾角 */
#define BALANCE_OUT_MIN         -40.0f  /* 最大反向倾角 */
#define BALANCE_SETPOINT_MM     0.0f     /* 目标位置：零点 */

/*---- 序列模式：开环踢球 + PID 接球 ---- */
/* 详见 balance.c 中 seq_state_t 状态机 */

/*---- 按键引脚（SysConfig 已自动初始化，使用生成的宏）---- */
/* PA18: 系统使能 */
#define BTN_ENABLE_PORT         GPIO_KEY_PORT
#define BTN_ENABLE_PIN          GPIO_KEY_PIN_BUTTON_PIN    /* PA18, PINCM40 */
#define BTN_ENABLE_PINCM        GPIO_KEY_PIN_BUTTON_IOMUX
/* PA13: Key1 — 序列 0→+50→-50 */
#define BTN_KEY1_PORT           GPIO_KEY_PORT
#define BTN_KEY1_PIN            GPIO_KEY_PIN_0_PIN         /* PA13, PINCM35 */
#define BTN_KEY1_PINCM          GPIO_KEY_PIN_0_IOMUX
/* PA12: Key2 — 归中 */
#define BTN_KEY2_PORT           GPIO_KEY_PORT
#define BTN_KEY2_PIN            GPIO_KEY_PIN_1_PIN         /* PA12, PINCM34 */
#define BTN_KEY2_PINCM          GPIO_KEY_PIN_1_IOMUX

/*---- Ring buffer ---- */
#define BALANCE_RBUF_SIZE       256

/*---- LED (SysConfig 已自动初始化) ---- */
#define LED_PORT                GPIO_LED_PORT
#define LED_PIN                 GPIO_LED_PIN_2_PIN          /* PA0, PINCM1 */
#define LED_ON()                DL_GPIO_setPins(LED_PORT, LED_PIN)
#define LED_OFF()               DL_GPIO_clearPins(LED_PORT, LED_PIN)
#define LED_TOGGLE()            DL_GPIO_togglePins(LED_PORT, LED_PIN)

/*==================== API ====================*/

void balance_init(void);
void balance_poll(void);
void balance_set_setpoint(float sp_mm);
int  balance_get_setpoint_mm(void);

/* 由主函数调用的控制接口 */
void    balance_enable(void);
void    balance_disable(void);
uint8_t balance_is_enabled(void);
void    balance_start_sequence(void);       /* 0→+50→-50 */
void    balance_cancel_sequence(void);      /* 取消序列，归中 */
uint8_t balance_is_sequence_running(void);  /* 序列是否执行中 */

#endif
