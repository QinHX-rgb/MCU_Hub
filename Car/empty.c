#include "ti_msp_dl_config.h"
#include <stdio.h>
#include "delay.h"
#include "uart.h"
#include "CY_Z.h"
#include "grayscale_sensor.h"
#include "motor.h"
#include "Motor_Control.h"
#include "PID.h"
#include "OLED.h"
#include "clock.h"
#include "Encoder.h"
#include "interrupt.h"
#include "Buzzer.h"

/*==================================================================
 *  main — 循线小车主函数
 *  上电 → 初始化 → 等待按键 → 倒计时 → 循迹循环
 *==================================================================*/
uint8_t oled_buffer[32];

int main(void)
{
    /*—————— 系统初始化 ——————*/
    SYSCFG_DL_init();               // SysConfig 生成的 GPIO/PWM/UART/SysTick 初始化
    UART_Init();                    // 调试串口 UART0 使能
    SysTick_Init();

    /* 外设初始化 */
    Grayscale_Sensor_Init();        // 74HC165 灰度传感器
    Encoder_Init();                 // 编码器 GPIO 中断 (PB0~PB3)
    Motor_Control_Init();           // 控制 PID 初始化（参数在 Motor_Control.c 各函数前）
    CY_Z_Init();                    // CY-Z 陀螺仪 UART1 接收
    Motor_Off();                    // 确保电机初始为停止状态
    OLED_Init();
    Interrupt_Init();
    Buzzer_Init();

    /*—————— 上电指示：LED 快闪 3 次 ——————*/
    for (uint8_t i = 0; i < 3; i++) {
        DL_GPIO_setPins(GPIO_LED_PORT, GPIO_LED_PIN_LED_PIN);
        delay_ms(100);
        DL_GPIO_clearPins(GPIO_LED_PORT, GPIO_LED_PIN_LED_PIN);
        delay_ms(100);
    }

    CY_Z_SendZeroAngleFixed();
    UART_Printf("请按S1开始循迹");

    /*—————— OLED 显示等待画面 ——————*/
    OLED_ShowString(0, 0, (uint8_t *)"Press S1", 16);
    OLED_ShowString(0, 2, (uint8_t *)"to Start", 16);

    /*—————— 等待按键启动 (PA18, 高电平有效, 内部下拉) ——————*/
    while (!DL_GPIO_readPins(GPIO_BUTTON_PIN_BUTTON_PORT,
                             GPIO_BUTTON_PIN_BUTTON_PIN)) {
        CY_Z_Telemetry telem;
        char buf[17];
        uint8_t d;

        Grayscale_Update();

        /* 第 3 行: 8 位灰度值 */
        d = g_sensor_raw_data;
        buf[0] = (d & 0x80) ? '1' : '0';
        buf[1] = (d & 0x40) ? '1' : '0';
        buf[2] = (d & 0x20) ? '1' : '0';
        buf[3] = (d & 0x10) ? '1' : '0';
        buf[4] = (d & 0x08) ? '1' : '0';
        buf[5] = (d & 0x04) ? '1' : '0';
        buf[6] = (d & 0x02) ? '1' : '0';
        buf[7] = (d & 0x01) ? '1' : '0';
        buf[8] = '\0';
        OLED_ShowString(0, 4, (uint8_t *)buf, 16);

        /* 第 4 行: 角度 */
        if (CY_Z_GetTelemetry(&telem))
            snprintf(buf, sizeof(buf), "Y=%-6.3f", telem.angle_deg);
        else
            snprintf(buf, sizeof(buf), "Y=--------");
        OLED_ShowString(0, 6, (uint8_t *)buf, 16);

        delay_ms(100);
    }
    delay_ms(20);   // 按键去抖

    OLED_Clear();

    /*—————— 启动倒计时：LED 慢闪 3 次 ——————*/
    for (uint8_t i = 0; i < 3; i++) {
        DL_GPIO_setPins(GPIO_LED_PORT, GPIO_LED_PIN_LED_PIN);
        delay_ms(300);
        DL_GPIO_clearPins(GPIO_LED_PORT, GPIO_LED_PIN_LED_PIN);
        delay_ms(300);
    }

    /*—————— 主控制循环 (~3ms 周期) ——————*/
    uint8_t beeped = 0;
    while (1)
    {
        static uint16_t disp_cnt = 0;
        char buf[17];
        CY_Z_Telemetry telem;

        Grayscale_Update();         // Flag: 0=停车 1=左转 2=右转 3=直行 4=循线
        Encoder_Update();

        if (Flag == 4) {                        // 正常循线
            Control_Line_Track();
            beeped = 0;
        } else if (Flag == 3) {                 // 全白 → 陀螺仪直行
            if (!beeped) { Buzzer_Beep(); beeped = 1; }
            Control_Straight();
        } else if (Flag == 2) {                 // 111xx000 → 右转
            Control_Corner(1);
            beeped = 0;
        } else if (Flag == 1) {                 // 000xx111 → 左转
            Control_Corner(-1);
            beeped = 0;
        } else {                                // Flag == 0 全黑 → 停车
            Motor_Off();
        }

        /* OLED 调试显示，~100ms 刷新一次 */
        if (++disp_cnt >= 30) {
            uint8_t d = g_sensor_raw_data;
            disp_cnt = 0;

            /* 第 1 行: 8 位二进制 + cx */
            buf[0] = (d & 0x80) ? '1' : '0';
            buf[1] = (d & 0x40) ? '1' : '0';
            buf[2] = (d & 0x20) ? '1' : '0';
            buf[3] = (d & 0x10) ? '1' : '0';
            buf[4] = (d & 0x08) ? '1' : '0';
            buf[5] = (d & 0x04) ? '1' : '0';
            buf[6] = (d & 0x02) ? '1' : '0';
            buf[7] = (d & 0x01) ? '1' : '0';
            buf[8] = ' ';
            snprintf(&buf[9], 8, "cx=%-3d", cx);
            OLED_ShowString(0, 0, (uint8_t *)buf, 16);

            /* 第 2 行: 直行参考 + 丢线次数 */
            snprintf(buf, sizeof(buf), "R=%-5.0f #%d",
                (double)Motor_Control_GetStraightRef(),
                Motor_Control_GetLostCount());
            OLED_ShowString(0, 2, (uint8_t *)buf, 16);

            /* 第 3 行: 实时角度 */
            if (CY_Z_GetTelemetry(&telem))
                snprintf(buf, sizeof(buf), "Yaw=%-6.3f", (double)telem.angle_deg);
            else
                snprintf(buf, sizeof(buf), "Yaw=--------");
            OLED_ShowString(0, 4, (uint8_t *)buf, 16);

            /* 第 4 行: 状态 */
            if (Flag == 4)
                snprintf(buf, sizeof(buf), "Track     ");
            else if (Flag == 3)
                snprintf(buf, sizeof(buf), "Straight  ");
            else if (Flag == 2)
                snprintf(buf, sizeof(buf), "RightTurn ");
            else if (Flag == 1)
                snprintf(buf, sizeof(buf), "LeftTurn  ");
            else
                snprintf(buf, sizeof(buf), "Stop      ");
            OLED_ShowString(0, 6, (uint8_t *)buf, 16);
        }

        delay_ms(3);                // 控制周期 ≈ 3ms (约 333Hz)
    }
}
