/*
 * balance.c - K230 $BALL 协议解析 + PID 舵机控制
 *
 * 协议帧格式: $BALL,序号,有效标志,位置mm,目标mm,置信度,帧间隔ms#\n
 *
 * 控制回路:
 *   position_mm → error = position - setpoint(0)
 *   → PID_Update(error) → angle = BALANCE_ANGLE + output
 *   → servo_set_angle(angle) clamped [45°, 140°]
 *
 * 方向约定:
 *   40° → 球右移(坐标增大); 角度增大 → 球左移
 *   球偏右(pos>0) → 增大角度 → 球滚回左 ✓
 */

#include "ti_msp_dl_config.h"
#include "balance.h"
#include "servo.h"
#include "PID.h"
#include "drivers/uart.h"
#include <string.h>   /* memset */
#include <stdlib.h>   /* strtof */

/*============================================================================
 * Ring Buffer (K230 UART1 RX)
 *============================================================================*/

static volatile uint8_t  rbuf[BALANCE_RBUF_SIZE];
static volatile uint16_t rbuf_wr = 0;
static volatile uint16_t rbuf_rd = 0;

static void rbuf_push(uint8_t byte)
{
    uint16_t next = (rbuf_wr + 1) % BALANCE_RBUF_SIZE;
    if (next != rbuf_rd) {          /* 非满 */
        rbuf[rbuf_wr] = byte;
        rbuf_wr = next;
    }
}

static uint8_t rbuf_pop(uint8_t *byte)
{
    if (rbuf_rd == rbuf_wr) return 0;  /* 空 */
    *byte = rbuf[rbuf_rd];
    rbuf_rd = (rbuf_rd + 1) % BALANCE_RBUF_SIZE;
    return 1;
}

/*============================================================================
 * Protocol Parser
 *============================================================================*/

static char     line_buf[128];
static uint8_t  line_idx = 0;
static ball_data_t g_ball;

/* 解析一行: "$BALL,帧号,有效标志,位置mm,目标mm,置信度,帧间隔ms#" */
static uint8_t parse_frame(const char *s, ball_data_t *out)
{
    const char *p = s;

    /* ---- 帧头 $BALL, ---- */
    if (p[0] != '$' || p[1] != 'B' || p[2] != 'A'
     || p[3] != 'L' || p[4] != 'L' || p[5] != ',')
        return 0;
    p += 6;

    /* ---- 序号 ---- */
    out->frame_num = 0;
    while (*p >= '0' && *p <= '9') {
        out->frame_num = out->frame_num * 10 + (*p - '0');
        p++;
    }
    if (*p != ',') return 0; p++;

    /* ---- 有效标志 ---- */
    out->valid = (*p == '1') ? 1 : 0;
    while (*p >= '0' && *p <= '9') p++;
    if (*p != ',') return 0; p++;

    /* ---- 位置 (mm, 带符号) ---- */
    out->position_mm = (float)strtof(p, (char **)&p);
    if (*p != ',') return 0; p++;

    /* ---- 目标 (mm, 带符号) ---- */
    out->target_mm = (float)strtof(p, (char **)&p);
    if (*p != ',') return 0; p++;

    /* ---- 置信度 ---- */
    out->confidence = (float)strtof(p, (char **)&p);
    if (*p != ',') return 0; p++;

    /* ---- 帧间隔 (ms) ---- */
    out->dt_ms = 0;
    while (*p >= '0' && *p <= '9') {
        out->dt_ms = out->dt_ms * 10 + (*p - '0');
        p++;
    }
    if (*p != '#') return 0;

    return 1;   /* 解析成功 */
}

/*============================================================================
 * PID Control
 *============================================================================*/

static PID_TypeDef pid_center;     /* Center 模式专用 PID */
static PID_TypeDef pid_seq;        /* 序列 CATCH 专用 PID */
static float      g_setpoint_mm  = BALANCE_SETPOINT_MM;
static uint32_t   g_frame_count  = 0;
static uint32_t   g_valid_count  = 0;
static uint32_t   g_lost_count   = 0;
static uint8_t    g_servo_angle  = BALANCE_ANGLE;   /* 最近一次舵机角度 */

/* ---- 序列状态机 ---- */
typedef enum {
    SEQ_IDLE,
    SEQ_KICK_DROP,      /* 降 20°（55→35），等球到 +30mm */
    SEQ_KICK_RAISE,     /* 抬 25°（35→60），等球到 +5mm */
    SEQ_CATCH           /* 回平 55° + PID，目标 -50mm */
} seq_state_t;

static seq_state_t g_seq_state = SEQ_IDLE;

/* ---- 使能状态 ---- */
static uint8_t g_enabled = 0;   /* 0=禁用, 1=启用 */

static void balance_update(float position_mm, PID_TypeDef *p)
{
    float error, output, angle_f;
    uint8_t angle;

    error  = position_mm - g_setpoint_mm;
    output = PID_Update(p, error);

    angle_f = (float)BALANCE_ANGLE + output;
    angle = (uint8_t)angle_f;

    servo_set_angle(angle);
    g_servo_angle = angle;
}

/*============================================================================
 * API
 *============================================================================*/

void balance_init(void)
{
    memset((void *)rbuf, 0, sizeof(rbuf));
    rbuf_wr = 0;
    rbuf_rd = 0;
    line_idx = 0;
    memset(&g_ball, 0, sizeof(g_ball));
    g_frame_count = 0;
    g_valid_count = 0;
    g_lost_count  = 0;
    g_servo_angle = BALANCE_ANGLE;

    g_setpoint_mm = BALANCE_SETPOINT_MM;

    /* 两个独立 PID，各用各的参数 */
    PID_Init(&pid_center,
             CENTER_KP, CENTER_KI, CENTER_KD,
             BALANCE_OUT_MAX, BALANCE_OUT_MIN);
    PID_Init(&pid_seq,
             SEQ_KP, SEQ_KI, SEQ_KD,
             BALANCE_OUT_MAX, BALANCE_OUT_MIN);

    /* 舵机归中 → 平衡角度 */
    servo_set_angle(BALANCE_ANGLE);

    /* 初始状态：禁用，等待主函数调用 balance_enable() */
    g_enabled        = 0;
    g_seq_state = SEQ_IDLE;
    balance_set_setpoint(0.0f);

    /* 启动信息 */
    uart_debug_puts("\r\n================================\r\n");
    uart_debug_puts("  Ball Balance Controller\r\n");
    uart_debug_printf("  Balance angle: %d deg\r\n", BALANCE_ANGLE);
    uart_debug_printf("  Servo range:   %d ~ %d deg\r\n",
                      SERVO_HARD_MIN, SERVO_HARD_MAX);
    uart_debug_printf("  Center: Kp=%.2f Ki=%.3f Kd=%.2f\r\n",
                      CENTER_KP, CENTER_KI, CENTER_KD);
    uart_debug_printf("  Seq:    Kp=%.2f Ki=%.3f Kd=%.2f\r\n",
                      SEQ_KP, SEQ_KI, SEQ_KD);
    uart_debug_puts("  K230 -> PB6/PB7 (UART1)\r\n");
    uart_debug_puts("  PC   <- PA10/PA11 (UART0)\r\n");
    uart_debug_puts("  Servo <- PA8 (TIMA0 PWM)\r\n");
    uart_debug_puts("================================\r\n\r\n");
}

void balance_poll(void)
{
    uint8_t byte;

    /* ---- 1. 从 K230 UART1 把数据全部收进 ring buffer ---- */
    while (DL_UART_Main_receiveDataCheck(UART_K230_INST, &byte)) {
        rbuf_push(byte);
    }

    /* ---- 2. 按字节解析帧 ---- */
    while (rbuf_pop(&byte)) {

        /* 以换行符为帧结束标志 */
        if (byte == '\n' || byte == '\r') {
            if (line_idx == 0) continue;   /* 空行 */

            line_buf[line_idx] = '\0';
            line_idx = 0;

            /* 尝试解析 $BALL,...# 帧 */
            if (parse_frame(line_buf, &g_ball)) {
                g_frame_count++;

                if (g_ball.valid) {
                    g_valid_count++;
                } else {
                    g_lost_count++;
                }

                /* ---- 序列控制（仅启用时）---- */
                if (g_enabled && g_seq_state != SEQ_IDLE) {
                    float pos = g_ball.position_mm;

                    switch (g_seq_state) {

                    case SEQ_KICK_DROP:
                        /* 开环：降 20°（55→35）→ 球右滚 */
                        servo_set_angle((uint8_t)(BALANCE_ANGLE - 15));
                        if (pos > 20.0f) {
                            g_seq_state = SEQ_KICK_RAISE;
                            uart_debug_puts("[seq] >+20mm, raise 25deg!\r\n");
                        }
                        break;

                    case SEQ_KICK_RAISE:
                        /* 开环：抬 25°（35→60）→ 球左滚 */
                        servo_set_angle((uint8_t)(BALANCE_ANGLE + 5));
                        if (pos < 5.0f) {
                            g_seq_state = SEQ_CATCH;
                            servo_set_angle(BALANCE_ANGLE);
                            /* PID 预置：目标 -50，当前误差 = pos - (-50) */
                            g_setpoint_mm = -50.0f;
                            pid_seq.last_error = pos - g_setpoint_mm;
                            uart_debug_printf("[seq] <+5mm, PID catch err=%.0f\r\n",
                                              (double)pid_seq.last_error);
                        }
                        break;

                    case SEQ_CATCH:
                        /* 序列 PID 接管，目标 -50mm */
                        balance_update(pos, &pid_seq);
                        break;

                    default: break;
                    }
                } else if (g_enabled) {
                    /* 启用但无序列 → Center 模式 */
                    balance_update(g_ball.position_mm, &pid_center);
                } else {
                    servo_set_angle(BALANCE_ANGLE);
                }

                /* 调试输出（每 10 个有效帧打印一次） */
                if (g_valid_count > 0 && g_valid_count % 10 == 0) {
                        uart_debug_printf(
                            "[%d] pos=%.1fmm  err=%.1fmm  angle=%d  conf=%.2f  dt=%dms\r\n",
                            (int)g_frame_count,
                            g_ball.position_mm,
                            g_ball.position_mm - g_setpoint_mm,
                            (int)g_servo_angle,
                            g_ball.confidence,
                            (int)g_ball.dt_ms);
                    }
                }
                continue;
            }

        /* 普通字节：累积到行缓冲（防溢出） */
        if (line_idx < sizeof(line_buf) - 1) {
            line_buf[line_idx++] = (char)byte;
        } else {
            line_idx = 0;   /* 行过长，丢弃 */
        }
    }
}

void balance_set_setpoint(float sp_mm)
{
    g_setpoint_mm = sp_mm;
    PID_Reset(&pid_center);
    uart_debug_printf("[balance] setpoint -> %.1fmm\r\n", sp_mm);
}

int balance_get_setpoint_mm(void)
{
    return (int)g_setpoint_mm;
}

void balance_enable(void)
{
    g_enabled = 1;
    g_seq_state = SEQ_IDLE;
    PID_Reset(&pid_center);
    g_setpoint_mm = 0.0f;
    uart_debug_puts("[balance] enabled\r\n");
}

void balance_disable(void)
{
    g_enabled = 0;
    g_seq_state = SEQ_IDLE;
    servo_set_angle(BALANCE_ANGLE);
    uart_debug_puts("[balance] disabled\r\n");
}

uint8_t balance_is_enabled(void)
{
    return g_enabled;
}

void balance_start_sequence(void)
{
    if (!g_enabled) return;
    g_seq_state = SEQ_KICK_DROP;
    uart_debug_puts("[seq] kick: drop -> raise -> flat -> catch @ -50mm\r\n");
}

void balance_cancel_sequence(void)
{
    g_seq_state = SEQ_IDLE;
    balance_set_setpoint(0.0f);
    uart_debug_puts("[seq] cancelled, holding center\r\n");
}

uint8_t balance_is_sequence_running(void)
{
    return (g_seq_state != SEQ_IDLE) ? 1 : 0;
}
