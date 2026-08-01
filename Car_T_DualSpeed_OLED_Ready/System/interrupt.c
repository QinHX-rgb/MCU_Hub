#include "ti_msp_dl_config.h"
#include "interrupt.h"
#include "clock.h"
#include "Encoder.h"

uint8_t enable_group1_irq = 0;

void Interrupt_Init(void)
{
    if(enable_group1_irq)
    {
        NVIC_EnableIRQ(1);
    }
}

void SysTick_Handler(void)
{
    tick_ms++;
}

void GROUP1_IRQHandler(void)
{
    switch (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1)) {
        #if defined GPIO_MULTIPLE_GPIOA_INT_IIDX
        case GPIO_MULTIPLE_GPIOA_INT_IIDX:
            switch (DL_GPIO_getPendingInterrupt(GPIOA))
            {
                #if (defined GPIO_MPU6050_PORT) && (GPIO_MPU6050_PORT == GPIOA)
                case GPIO_MPU6050_PIN_MPU6050_INT_IIDX:
                    Read_Quad();
                    break;
                #endif

                #if (defined GPIO_LSM6DSV16X_PORT) && (GPIO_LSM6DSV16X_PORT == GPIOA)
                case GPIO_LSM6DSV16X_PIN_LSM6DSV16X_INT_IIDX:
                    Read_LSM6DSV16X();
                    break;
                #endif

                #if (defined GPIO_VL53L0X_PIN_VL53L0X_GPIO1_PORT) && (GPIO_VL53L0X_PIN_VL53L0X_GPIO1_PORT == GPIOA)
                case GPIO_VL53L0X_PIN_VL53L0X_GPIO1_IIDX:
                    Read_VL53L0X();
                    break;
                #endif

                #if (defined GPIO_IMU660RB_PIN_IMU660RB_INT1_PORT) && (GPIO_IMU660RB_PIN_IMU660RB_INT1_PORT == GPIOA)
                case GPIO_IMU660RB_PIN_IMU660RB_INT1_IIDX:
                    Read_IMU660RB();
                    break;
                #endif

                default:
                    break;
            }
            break;
        #endif

        #if defined GPIO_MULTIPLE_GPIOB_INT_IIDX
        case GPIO_MULTIPLE_GPIOB_INT_IIDX:
            switch (DL_GPIO_getPendingInterrupt(GPIOB))
            {
                #if (defined GPIO_MPU6050_PORT) && (GPIO_MPU6050_PORT == GPIOB)
                case GPIO_MPU6050_PIN_MPU6050_INT_IIDX:
                    Read_Quad();
                    break;
                #endif

                #if (defined GPIO_LSM6DSV16X_PORT) && (GPIO_LSM6DSV16X_PORT == GPIOB)
                case GPIO_LSM6DSV16X_PIN_LSM6DSV16X_INT_IIDX:
                    Read_LSM6DSV16X();
                    break;
                #endif

                #if (defined GPIO_VL53L0X_PIN_VL53L0X_GPIO1_PORT) && (GPIO_VL53L0X_PIN_VL53L0X_GPIO1_PORT == GPIOB)
                case GPIO_VL53L0X_PIN_VL53L0X_GPIO1_IIDX:
                    Read_VL53L0X();
                    break;
                #endif

                #if (defined GPIO_IMU660RB_PIN_IMU660RB_INT1_PORT) && (GPIO_IMU660RB_PIN_IMU660RB_INT1_PORT == GPIOB)
                case GPIO_IMU660RB_PIN_IMU660RB_INT1_IIDX:
                    Read_IMU660RB();
                    break;
                #endif

                /* 编码器: PB0=右A, PB12=左A — X2 解码 */
                case DL_GPIO_IIDX_DIO0:
                    if (DL_GPIO_readPins(ENC_R_A_PORT, ENC_R_A_PIN)) {
                        if (DL_GPIO_readPins(ENC_R_B_PORT, ENC_R_B_PIN))
                            g_enc_R_pulses++;
                        else
                            g_enc_R_pulses--;
                    } else {
                        if (DL_GPIO_readPins(ENC_R_B_PORT, ENC_R_B_PIN))
                            g_enc_R_pulses--;
                        else
                            g_enc_R_pulses++;
                    }
                    DL_GPIO_clearInterruptStatus(GPIOB, ENC_R_A_PIN);
                    break;

                case DL_GPIO_IIDX_DIO12:
                    if (DL_GPIO_readPins(ENC_L_A_PORT, ENC_L_A_PIN)) {
                        if (DL_GPIO_readPins(ENC_L_B_PORT, ENC_L_B_PIN))
                            g_enc_L_pulses++;
                        else
                            g_enc_L_pulses--;
                    } else {
                        if (DL_GPIO_readPins(ENC_L_B_PORT, ENC_L_B_PIN))
                            g_enc_L_pulses--;
                        else
                            g_enc_L_pulses++;
                    }
                    DL_GPIO_clearInterruptStatus(GPIOB, ENC_L_A_PIN);
                    break;

                default:
                    break;
            }
            break;
        #endif

        #if defined GPIO_MPU6050_INT_IIDX
            case GPIO_MPU6050_INT_IIDX:
                Read_Quad();
                break;
        #endif

        #if defined GPIO_LSM6DSV16X_INT_IIDX
            case GPIO_LSM6DSV16X_INT_IIDX:
                Read_LSM6DSV16X();
                break;
        #endif

        #if defined GPIO_VL53L0X_INT_IIDX
            case GPIO_VL53L0X_INT_IIDX:
                Read_VL53L0X();
                break;
        #endif

        #if defined GPIO_IMU660RB_INT_IIDX
            case GPIO_IMU660RB_INT_IIDX:
                Read_IMU660RB();
                break;
        #endif
    }
}
