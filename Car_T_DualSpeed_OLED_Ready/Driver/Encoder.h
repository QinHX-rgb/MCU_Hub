#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

/*==================================================================
 *  双路编码器 — AB 相正交解码测速
 *
 *  右编码器: A=PB0 (Conder PIN_5) / B=PB6 (Conder PIN_6)
 *  左编码器: A=PB12(Conder PIN_3) / B=PA13(Conder PIN_4)
 *==================================================================*/

/*—— 右编码器引脚宏 ——*/
#define ENC_R_A_PORT    (GPIO_Conder_PIN_5_PORT)   /* GPIOB        */
#define ENC_R_A_PIN     (GPIO_Conder_PIN_5_PIN)    /* DL_GPIO_PIN_0 */
#define ENC_R_A_IOMUX   (GPIO_Conder_PIN_5_IOMUX)  /* IOMUX_PINCM12 */

#define ENC_R_B_PORT    (GPIO_Conder_PIN_6_PORT)   /* GPIOB        */
#define ENC_R_B_PIN     (GPIO_Conder_PIN_6_PIN)    /* DL_GPIO_PIN_6 */
#define ENC_R_B_IOMUX   (GPIO_Conder_PIN_6_IOMUX)  /* IOMUX_PINCM23 */

/*—— 左编码器引脚宏 ——*/
#define ENC_L_A_PORT    (GPIO_Conder_PIN_3_PORT)   /* GPIOB         */
#define ENC_L_A_PIN     (GPIO_Conder_PIN_3_PIN)    /* DL_GPIO_PIN_12 */
#define ENC_L_A_IOMUX   (GPIO_Conder_PIN_3_IOMUX)  /* IOMUX_PINCM29  */

#define ENC_L_B_PORT    (GPIO_Conder_PIN_4_PORT)   /* GPIOA         */
#define ENC_L_B_PIN     (GPIO_Conder_PIN_4_PIN)    /* DL_GPIO_PIN_13 */
#define ENC_L_B_IOMUX   (GPIO_Conder_PIN_4_IOMUX)  /* IOMUX_PINCM35  */

/* ISR 写入的脉冲累计值 */
extern volatile int16_t g_enc_R_pulses;
extern volatile int16_t g_enc_L_pulses;

void    Encoder_Init(void);
void    Encoder_Poll(void);    // 轮询编码器引脚 (每控制周期调用)
void    Encoder_Update(void);
int16_t Encoder_GetSpeedL(void);
int16_t Encoder_GetSpeedR(void);

#endif
