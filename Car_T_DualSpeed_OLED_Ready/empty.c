#include "ti_msp_dl_config.h"
#include <stdbool.h>
#include <stdio.h>
#include "delay.h"
#include "uart.h"
#include "grayscale_sensor.h"
#include "motor.h"
#include "Motor_Control.h"
#include "app_config.h"
#include "OLED.h"
#include "clock.h"
#include "Encoder.h"
#include "interrupt.h"

static void DisplaySensorBits(void)
{
#if ENABLE_OLED
    char buf[17];
    uint8_t data;

    Grayscale_Update();
    data = g_sensor_raw_data;
    for (uint8_t i = 0; i < 8; i++) {
        buf[i] = (data & (1U << (7U - i))) ? '1' : '0';
    }
    buf[8] = '\0';
    OLED_ShowString(0, 4, (uint8_t *)buf, 16);
#else
    Grayscale_Update();
#endif
}

int main(void)
{
    uint16_t display_count = 0;
    uint8_t start_line_clear_frames = 0;
    uint8_t max_black_count = 0;
    bool stop_line_armed = false;
    bool fast_mode = true;

    /* 计时器相关变量 */
    unsigned long run_start_ms = 0;
    unsigned long elapsed_ms = 0;
    unsigned long elapsed_sec = 0;
    unsigned long elapsed_min = 0;
    unsigned long elapsed_tenth = 0;

    SYSCFG_DL_init();
    Encoder_Init();
    UART_Init();
    SysTick_Init();
    Grayscale_Sensor_Init();
    Motor_Init();
    Motor_Off();

#if ENABLE_OLED
    OLED_Init();
    OLED_Display_On();
#endif
    Interrupt_Init();

    DL_GPIO_clearPins(GPIO_LED_PORT, GPIO_LED_PIN_LED_PIN);

    /* ================================================================
     * 主循环 — 支持无限次重新发车
     *
     * 上电显示 "START" + "K1:SLOW K2:FAST"；
     * 按 Key1 选择慢速 / Key2 选择快速 → OLED 确认档位 + "START:S1"；
     * 按 S1(PA18) 发车 → 行驶中 OLED 显示计时器 + 8 路传感器二进制；
     * 跑到停车线后停车显示成绩，再次按键重新发车。
     * ================================================================ */
    while (1) {
        /* ---- 每次发车前复位运行状态 ---- */
        start_line_clear_frames = 0;
        max_black_count = 0;
        stop_line_armed = false;
        display_count = 0;

        Motor_ResetControl();
        Motor_Off();
        DL_GPIO_clearPins(GPIO_LED_PORT, GPIO_LED_PIN_LED_PIN);

        /* 清零编码器累计脉冲 */
        __disable_irq();
        g_enc_R_pulses = 0;
        g_enc_L_pulses = 0;
        __enable_irq();

#if ENABLE_OLED
        OLED_ShowString(0, 0, (uint8_t *)"---- START ---- ", 16);
        OLED_ShowString(0, 2, (uint8_t *)"K1:SLOW K2:FAST", 16);
        OLED_ShowString(0, 4, (uint8_t *)"                ", 16);
        OLED_ShowString(0, 6, (uint8_t *)"                ", 16);
#endif

        /* ---- 阶段 1a：等待 Key1/Key2 选择速度档位 ---- */
        while (1) {
            /* Key1 / PA27 → 慢速档，低电平有效 */
            if (!DL_GPIO_readPins(GPIO_BUTTON_PIN_Key1_PORT,
                                  GPIO_BUTTON_PIN_Key1_PIN)) {
                delay_ms(10);
                if (!DL_GPIO_readPins(GPIO_BUTTON_PIN_Key1_PORT,
                                      GPIO_BUTTON_PIN_Key1_PIN)) {
                    fast_mode = false;
#if ENABLE_OLED
                    OLED_ShowString(0, 0, (uint8_t *)"SLOW MODE       ", 16);
                    OLED_ShowString(0, 2, (uint8_t *)"START: S1       ", 16);
#endif
                    break;  /* 已选择慢速 → 等待 S1 确认发车 */
                }
            }

            /* Key2 / PB2 → 快速档，低电平有效 */
            if (!DL_GPIO_readPins(GPIO_BUTTON_PIN_Key2_PORT,
                                  GPIO_BUTTON_PIN_Key2_PIN)) {
                delay_ms(10);
                if (!DL_GPIO_readPins(GPIO_BUTTON_PIN_Key2_PORT,
                                      GPIO_BUTTON_PIN_Key2_PIN)) {
                    fast_mode = true;
#if ENABLE_OLED
                    OLED_ShowString(0, 0, (uint8_t *)"FAST MODE       ", 16);
                    OLED_ShowString(0, 2, (uint8_t *)"START: S1       ", 16);
#endif
                    break;  /* 已选择快速 → 等待 S1 确认发车 */
                }
            }

            DisplaySensorBits();
            delay_ms(5);
        }

        /* ---- 阶段 1b：等待 S1 按键确认发车 ---- */
        while (1) {
            /* S1 / PA18 → 发车，高电平有效 */
            if (DL_GPIO_readPins(GPIO_BUTTON_PIN_BUTTON_PORT,
                                 GPIO_BUTTON_PIN_BUTTON_PIN)) {
                delay_ms(20);
                if (DL_GPIO_readPins(GPIO_BUTTON_PIN_BUTTON_PORT,
                                     GPIO_BUTTON_PIN_BUTTON_PIN)) {
                    while (DL_GPIO_readPins(
                               GPIO_BUTTON_PIN_BUTTON_PORT,
                               GPIO_BUTTON_PIN_BUTTON_PIN)) {
                        delay_ms(10);
                    }
                    delay_ms(20);
                    break;  /* S1 按下 → 发车！ */
                }
            }

            delay_ms(5);
        }

        /* ---- 阶段 2：按键已按下，记录起始时间并发车 ---- */
        run_start_ms = tick_ms;

        Motor_SetSpeedFast(fast_mode);
        Motor_On();

#if ENABLE_OLED
        OLED_ShowString(0, 0,
            (uint8_t *)(fast_mode ? "FAST MODE       " : "SLOW MODE       "),
            16);
        OLED_ShowString(0, 2, (uint8_t *)"RUNNING...      ", 16);
        OLED_ShowString(0, 4, (uint8_t *)"                ", 16);
        OLED_ShowString(0, 6, (uint8_t *)"                ", 16);
#endif

        /* ---- 阶段 3：行驶中，直到检测到停车线 ---- */
        while (1) {
#if ENABLE_OLED
            char buf[17];
#endif
            uint8_t black_count;

            Encoder_Poll();
            black_count = Grayscale_Update();
            Encoder_Update();

            /*
             * 起跑时忽略当前启停线。只有连续看到 1~2 路黑色的普通
             * 轨迹后才武装，证明传感器已经完整离开起点横线。
             */
            if (!stop_line_armed) {
                if ((black_count > 0U) &&
                    (black_count < STOP_LINE_BLACK_MIN)) {
                    if (++start_line_clear_frames >=
                        START_LINE_CLEAR_FRAMES) {
                        stop_line_armed = true;
#if ENABLE_UART_DEBUG
                        UART_Printf("STOP detector armed");
#endif
                    }
                } else {
                    start_line_clear_frames = 0;
                }
            } else {
                if (black_count > max_black_count) {
                    max_black_count = black_count;
                }

                bool wide_stop_line =
                    (black_count >= STOP_LINE_BLACK_MIN);

                if (wide_stop_line) {
                    /*
                     * 通道 0 保持较弱、较短制动并先释放，避免物理右轮
                     * 反转；只增强通道 1 的制动力来缩短停车距离。
                     */
                    Motor_Brake();
                    delay_ms(FINISH_BRAKE_TIME_MS);
                    

                    Motor_Off();
                    DL_GPIO_setPins(GPIO_LED_PORT, GPIO_LED_PIN_LED_PIN);

                    /* 计算最终用时 */
                    elapsed_ms = tick_ms - run_start_ms;
                    elapsed_sec = elapsed_ms / 1000UL;
                    elapsed_min = elapsed_sec / 60UL;
                    elapsed_sec = elapsed_sec % 60UL;
                    elapsed_tenth = (elapsed_ms / 100UL) % 10UL;

#if ENABLE_UART_DEBUG
                    UART_Printf("FINISH: B=%u M=%u Time=%lu:%02lu.%lu",
                                black_count, max_black_count,
                                elapsed_min, elapsed_sec, elapsed_tenth);
#endif
#if ENABLE_OLED
                    OLED_ShowString(0, 0, (uint8_t *)"====FINISHED==== ", 16);
                    snprintf(buf, sizeof(buf), "TIME %02lu:%02lu.%lu  ",
                             elapsed_min, elapsed_sec, elapsed_tenth);
                    OLED_ShowString(0, 2, (uint8_t *)buf, 16);
                    OLED_ShowString(0, 4,
                        (uint8_t *)(fast_mode ? "MODE:FAST       " :
                                                 "MODE:SLOW       "),
                        16);
                    OLED_ShowString(0, 6, (uint8_t *)"Press to restart", 16);
#endif
                    /* 等待按键选择模式 → 重新发车 */
                    while (1) {
                        /* Key1 / PA27 → 慢速档，低电平有效 */
                        if (!DL_GPIO_readPins(GPIO_BUTTON_PIN_Key1_PORT,
                                              GPIO_BUTTON_PIN_Key1_PIN)) {
                            delay_ms(10);
                            if (!DL_GPIO_readPins(GPIO_BUTTON_PIN_Key1_PORT,
                                                  GPIO_BUTTON_PIN_Key1_PIN)) {
                                fast_mode = false;
                            }
                        }

                        /* Key2 / PB2 → 快速档，低电平有效 */
                        if (!DL_GPIO_readPins(GPIO_BUTTON_PIN_Key2_PORT,
                                              GPIO_BUTTON_PIN_Key2_PIN)) {
                            delay_ms(10);
                            if (!DL_GPIO_readPins(GPIO_BUTTON_PIN_Key2_PORT,
                                                  GPIO_BUTTON_PIN_Key2_PIN)) {
                                fast_mode = true;
                            }
                        }

                        /* S1 / PA18 → 发车，高电平有效 */
                        if (DL_GPIO_readPins(GPIO_BUTTON_PIN_BUTTON_PORT,
                                             GPIO_BUTTON_PIN_BUTTON_PIN)) {
                            delay_ms(20);
                            if (DL_GPIO_readPins(GPIO_BUTTON_PIN_BUTTON_PORT,
                                                 GPIO_BUTTON_PIN_BUTTON_PIN)) {
                                while (DL_GPIO_readPins(
                                           GPIO_BUTTON_PIN_BUTTON_PORT,
                                           GPIO_BUTTON_PIN_BUTTON_PIN)) {
                                    delay_ms(10);
                                }
                                delay_ms(20);
                                break;
                            }
                        }

                        delay_ms(50);
                    }
                    /* 跳出行驶循环 → 回到主循环顶部重新发车 */
                    break;
                }
            }

            /* 全程只使用灰度重心循迹，不再切换到丢线、直角或陀螺仪模式。 */
            Motor_FollowLine(cx);

            /* ---- OLED 显示刷新（含实时计时器） ---- */
            if (++display_count >= DISPLAY_UPDATE_FRAMES) {
                display_count = 0;

                /* 实时计时 */
                elapsed_ms = tick_ms - run_start_ms;
                elapsed_sec = elapsed_ms / 1000UL;
                elapsed_min = elapsed_sec / 60UL;
                elapsed_sec = elapsed_sec % 60UL;
                elapsed_tenth = (elapsed_ms / 100UL) % 10UL;

#if ENABLE_OLED
                /* Line 0: 模式 + 实时计时器 */
                snprintf(buf, sizeof(buf), "%s %02lu:%02lu.%lu  ",
                         fast_mode ? "FAST" : "SLOW",
                         elapsed_min, elapsed_sec, elapsed_tenth);
                OLED_ShowString(0, 0, (uint8_t *)buf, 16);

                /* Line 2: 8 路灰度传感器二进制值 */
                {
                    uint8_t raw = g_sensor_raw_data;
                    for (uint8_t i = 0; i < 8; i++) {
                        buf[i] = (raw & (1U << (7U - i))) ? '1' : '0';
                    }
                    buf[8] = '\0';
                }
                OLED_ShowString(0, 2, (uint8_t *)buf, 16);

                /* Line 4: 黑线数 + 最大黑线数 + 武装状态 */
                snprintf(buf, sizeof(buf), "B=%u M=%u %s     ",
                         black_count, max_black_count,
                         stop_line_armed ? "ARM" : "WAIT");
                OLED_ShowString(0, 4, (uint8_t *)buf, 16);

                /* Line 6: 基础 PWM 占空比 */
                snprintf(buf, sizeof(buf), "PWM=%u          ",
                         fast_mode ? BASE_PWM_FAST : BASE_PWM_SLOW);
                OLED_ShowString(0, 6, (uint8_t *)buf, 16);
#endif
#if ENABLE_UART_DEBUG
                UART_Printf("B=%u M=%u ARM=%u T=%lu.%lus",
                            black_count, max_black_count,
                            stop_line_armed ? 1U : 0U,
                            elapsed_ms / 1000UL,
                            elapsed_ms % 1000UL);
#endif
                if (stop_line_armed) {
                    DL_GPIO_togglePins(GPIO_LED_PORT,
                                       GPIO_LED_PIN_LED_PIN);
                }
            }

            delay_ms(CONTROL_PERIOD_MS);
        }
        /* 行驶循环结束 → 回到主循环 while(1) 顶部，重新等待发车按键 */
    }
}
