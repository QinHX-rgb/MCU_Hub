#include "Encoder.h"

/* 速度滤波系数 (EMA) */
#define ENC_FILT        0.3f

/*==================================================================
 *  模块内变量
 *==================================================================*/
volatile int16_t g_enc_R_pulses;   // 右轮本周期脉冲累计
volatile int16_t g_enc_L_pulses;   // 左轮本周期脉冲累计

static float g_enc_R_speed;        // 右轮滤波速度 (脉冲/周期)
static float g_enc_L_speed;        // 左轮滤波速度 (脉冲/周期)

/* 轮询模式: 记录上一次 A 相电平, 用于检测边沿 */
static uint8_t g_enc_R_A_last = 0;
static uint8_t g_enc_L_A_last = 0;
static bool    g_enc_first    = true;

/*==================================================================
 *  Encoder_Init — 配置编码器引脚
 *==================================================================*/
void Encoder_Init(void)
{
    /* 右 A (PB0): 输入+上拉 */
    DL_GPIO_initDigitalInputFeatures(ENC_R_A_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    /* 右 B (PB6): 纯输入+上拉 */
    DL_GPIO_initDigitalInputFeatures(ENC_R_B_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    /* 左 A (PB12): 输入+上拉 */
    DL_GPIO_initDigitalInputFeatures(ENC_L_A_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    /* 左 B (PA13): 纯输入+上拉 */
    DL_GPIO_initDigitalInputFeatures(ENC_L_B_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    /* SysConfig 中这组通用 GPIO 的默认方向是输出。这里必须明确关闭
     * 四路输出驱动，避免开发板与编码器的 A/B 输出发生电平冲突。
     * 引脚编号不变，仅将电气方向修正为输入。 */
    DL_GPIO_disableOutput(ENC_R_A_PORT, ENC_R_A_PIN);
    DL_GPIO_disableOutput(ENC_R_B_PORT, ENC_R_B_PIN);
    DL_GPIO_disableOutput(ENC_L_A_PORT, ENC_L_A_PIN);
    DL_GPIO_disableOutput(ENC_L_B_PORT, ENC_L_B_PIN);

    /* 计数清零 */
    g_enc_R_pulses = 0;
    g_enc_L_pulses = 0;
    g_enc_R_speed  = 0.0f;
    g_enc_L_speed  = 0.0f;
    g_enc_first    = true;
}

/*==================================================================
 *  Encoder_Poll — 轮询编码器 (每控制周期调用一次)
 *
 *  手动读 A 相引脚, 检测边沿变化, 配合 B 相判断方向。
 *  3ms 周期轮询足够捕捉低速编码器 (约 100 脉冲/秒以下)。
 *==================================================================*/
void Encoder_Poll(void)
{
    uint8_t rA, lA, rB, lB;

    /* 读当前电平 */
    rA = (uint8_t)!!DL_GPIO_readPins(ENC_R_A_PORT, ENC_R_A_PIN);
    lA = (uint8_t)!!DL_GPIO_readPins(ENC_L_A_PORT, ENC_L_A_PIN);

    /* 首次调用: 只记录初始电平, 不计数 */
    if (g_enc_first) {
        g_enc_R_A_last = rA;
        g_enc_L_A_last = lA;
        g_enc_first = false;
        return;
    }

    /* —— 右编码器 ——— */
    if (rA != g_enc_R_A_last) {
        /* A 相电平变化 → 边沿! */
        rB = (uint8_t)!!DL_GPIO_readPins(ENC_R_B_PORT, ENC_R_B_PIN);
        if (rA) {  /* 上升沿 */
            if (rB) g_enc_R_pulses++;
            else    g_enc_R_pulses--;
        } else {   /* 下降沿 */
            if (rB) g_enc_R_pulses--;
            else    g_enc_R_pulses++;
        }
        g_enc_R_A_last = rA;
    }

    /* —— 左编码器 —— */
    if (lA != g_enc_L_A_last) {
        lB = (uint8_t)!!DL_GPIO_readPins(ENC_L_B_PORT, ENC_L_B_PIN);
        if (lA) {
            if (lB) g_enc_L_pulses++;
            else    g_enc_L_pulses--;
        } else {
            if (lB) g_enc_L_pulses--;
            else    g_enc_L_pulses++;
        }
        g_enc_L_A_last = lA;
    }
}

/*==================================================================
 *  Encoder_Update — 计算速度 + 清零脉冲
 *==================================================================*/
void Encoder_Update(void)
{
    int16_t r, l;

    __disable_irq();
    r = g_enc_R_pulses;
    l = g_enc_L_pulses;
    g_enc_R_pulses = 0;
    g_enc_L_pulses = 0;
    __enable_irq();

    g_enc_R_speed += ENC_FILT * ((float)r - g_enc_R_speed);
    g_enc_L_speed += ENC_FILT * ((float)l - g_enc_L_speed);
}

/*==================================================================
 *  Encoder_GetSpeedL / R
 *==================================================================*/
int16_t Encoder_GetSpeedL(void)
{
    return (int16_t)g_enc_L_speed;
}

int16_t Encoder_GetSpeedR(void)
{
    return (int16_t)g_enc_R_speed;
}
