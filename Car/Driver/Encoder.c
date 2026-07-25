#include "Encoder.h"

/* 速度滤波系数 (EMA) */
#define ENC_FILT        0.3f

/*==================================================================
 *  模块内变量
 *==================================================================*/
volatile int16_t g_enc_R_pulses;   // 右轮本周期脉冲累计 (ISR 写)
volatile int16_t g_enc_L_pulses;   // 左轮本周期脉冲累计 (ISR 写)

static float g_enc_R_speed;               // 右轮滤波速度 (脉冲/周期)
static float g_enc_L_speed;               // 左轮滤波速度 (脉冲/周期)

/*==================================================================
 *  Encoder_Init — 配置编码器引脚 + 中断
 *==================================================================*/
void Encoder_Init(void)
{
    /* 右 A (PB0): 输入+上拉+双沿中断 */
    DL_GPIO_initDigitalInputFeatures(ENC_R_A_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_setLowerPinsPolarity(ENC_R_A_PORT,
        (0x3U << (0U * 2U)));   /* pin0 双沿: bit[1:0]=11 */

    /* 右 B (PB6): 纯输入+上拉 */
    DL_GPIO_initDigitalInputFeatures(ENC_R_B_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    /* 左 A (PB12): 输入+上拉+双沿中断 */
    DL_GPIO_initDigitalInputFeatures(ENC_L_A_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_setLowerPinsPolarity(ENC_L_A_PORT,
        (0x3U << (12U * 2U)));  /* pin12 双沿: bit[25:24]=11 */

    /* 左 B (PA13): 纯输入+上拉 */
    DL_GPIO_initDigitalInputFeatures(ENC_L_B_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    /* 中断使能 */
    DL_GPIO_enableInterrupt(ENC_R_A_PORT, ENC_R_A_PIN);
    DL_GPIO_enableInterrupt(ENC_L_A_PORT, ENC_L_A_PIN);
    NVIC_ClearPendingIRQ(GPIOB_INT_IRQn);
    NVIC_EnableIRQ(GPIOB_INT_IRQn);

    /* 计数清零 */
    g_enc_R_pulses = 0;
    g_enc_L_pulses = 0;
    g_enc_R_speed  = 0.0f;
    g_enc_L_speed  = 0.0f;
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

