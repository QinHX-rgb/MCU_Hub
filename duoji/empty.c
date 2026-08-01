/*
 * empty.c - 小球平衡控制 主函数
 *
 * 按键 + LED 逻辑在这里。
 * PID / 舵机 / 序列控制通过 balance.h API 调用。
 */

#include "ti_msp_dl_config.h"
#include "balance.h"
#include "servo.h"
#include "delay.h"

/*============================================================================
 * 系统状态机
 *============================================================================*/

typedef enum {
    SYS_SELECT,
    SYS_ARMED,
    SYS_RUNNING
} sys_state_t;

typedef enum {
    MODE_NONE,
    MODE_SEQUENCE,
    MODE_CENTER
} run_mode_t;

static sys_state_t g_state = SYS_SELECT;
static run_mode_t  g_mode  = MODE_NONE;

/* ---- 按键防抖 ---- */
static uint8_t  g_btn_enable_prev = 0;
static uint8_t  g_btn_key1_prev   = 0;
static uint8_t  g_btn_key2_prev   = 0;
static uint16_t g_debounce        = 0;     /* 冷却计数 */

/*============================================================================
 * 按键检测（每 ~1ms 调用一次，debounce ~30ms）
 *============================================================================*/

static void btn_poll(void)
{
    uint8_t e_now  = DL_GPIO_readPins(GPIO_KEY_PORT,
                                      GPIO_KEY_PIN_BUTTON_PIN) ? 1 : 0;
    uint8_t k1_now = DL_GPIO_readPins(GPIO_KEY_PORT,
                                      GPIO_KEY_PIN_0_PIN) ? 1 : 0;
    uint8_t k2_now = DL_GPIO_readPins(GPIO_KEY_PORT,
                                      GPIO_KEY_PIN_1_PIN) ? 1 : 0;

    /* 冷却中跳过 */
    if (g_debounce > 0) {
        g_debounce--;
        g_btn_enable_prev = e_now;
        g_btn_key1_prev   = k1_now;
        g_btn_key2_prev   = k2_now;
        return;
    }

    switch (g_state) {

    case SYS_SELECT:
        /* PA13/PA12: 上拉，按下=低电平 → 下降沿 */
        if (g_btn_key1_prev == 1 && k1_now == 0) {
            g_mode  = MODE_SEQUENCE;
            g_state = SYS_ARMED;
            DL_GPIO_setPins(GPIO_LED_PORT, GPIO_LED_PIN_2_PIN);
            g_debounce = 30;
        }
        if (g_btn_key2_prev == 1 && k2_now == 0) {
            g_mode  = MODE_CENTER;
            g_state = SYS_ARMED;
            DL_GPIO_setPins(GPIO_LED_PORT, GPIO_LED_PIN_2_PIN);
            g_debounce = 30;
        }
        break;

    case SYS_ARMED:
        /* PA18: 下拉，按下=高电平 → 上升沿 */
        if (g_btn_enable_prev == 0 && e_now == 1) {
            g_state = SYS_RUNNING;
            DL_GPIO_clearPins(GPIO_LED_PORT, GPIO_LED_PIN_2_PIN);
            balance_enable();
            if (g_mode == MODE_SEQUENCE) balance_start_sequence();
            g_debounce = 30;
        }
        break;

    case SYS_RUNNING:
        if (g_btn_enable_prev == 0 && e_now == 1) {
            g_state = SYS_SELECT;
            g_mode  = MODE_NONE;
            DL_GPIO_clearPins(GPIO_LED_PORT, GPIO_LED_PIN_2_PIN);
            balance_disable();
            g_debounce = 30;
        }
        if (g_btn_key1_prev == 1 && k1_now == 0) {
            g_mode = MODE_SEQUENCE;
            balance_start_sequence();
            g_debounce = 30;
        }
        if (g_btn_key2_prev == 1 && k2_now == 0) {
            g_mode = MODE_CENTER;
            balance_cancel_sequence();
            g_debounce = 30;
        }
        break;

    default:
        break;
    }

    g_btn_enable_prev = e_now;
    g_btn_key1_prev   = k1_now;
    g_btn_key2_prev   = k2_now;
}

/*============================================================================
 * main
 *============================================================================*/

int main(void)
{
    SYSCFG_DL_init();

    /* 舵机立刻归中 */
    servo_set_angle(BALANCE_ANGLE);

    /* PA18 补充下拉（按下=高电平），PA12/PA13 保持 SysConfig 上拉（按下=低电平） */
    DL_GPIO_setDigitalInternalResistor(GPIO_KEY_PIN_BUTTON_IOMUX,
                                       DL_GPIO_RESISTOR_PULL_DOWN);

    /* 平衡模块初始化 */
    balance_init();

    /* 主循环 */
    while (1) {
        btn_poll();
        balance_poll();
    }
}
