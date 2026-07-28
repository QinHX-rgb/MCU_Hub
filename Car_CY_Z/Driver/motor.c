#include "motor.h"

/* 传感器全局变量（定义在 grayscale_sensor.c） */
extern int16_t cx;
extern uint8_t Flag;

/**
  * @brief			电机启动
  * @param		    None
  * @retval 		None
  */
void Motor_On(void)
{
    DL_GPIO_setPins(GPIO_Motor_PIN_STBY_PORT, GPIO_Motor_PIN_STBY_PIN);//置位对应端口->引脚电平,STBY
}

/**
  * @brief		    电机关闭
  * @param		    None
  * @retval 		None
  */
void Motor_Off(void)
{
    DL_GPIO_clearPins(GPIO_Motor_PIN_STBY_PORT, GPIO_Motor_PIN_STBY_PIN);//清除对应端口->引脚电平，STBY
    //逻辑口电平清除
    DL_GPIO_clearPins(GPIO_Motor_PIN_L1_PORT, GPIO_Motor_PIN_L1_PIN);
    DL_GPIO_clearPins(GPIO_Motor_PIN_L2_PORT, GPIO_Motor_PIN_L2_PIN);
    DL_GPIO_clearPins(GPIO_Motor_PIN_R1_PORT, GPIO_Motor_PIN_R1_PIN);
    DL_GPIO_clearPins(GPIO_Motor_PIN_R2_PORT, GPIO_Motor_PIN_R2_PIN);
}

/**
  * @brief			速度设置
  * @param		    左右轮占空比（百分值）
  * @retval 	    None
  */
#define PWM_PERIOD  1000

void Set_Speed(uint8_t side, int8_t duty)
{
    uint32_t cc_index;
    uint32_t abs_duty = (duty < 0) ? (uint32_t)(-duty) : (uint32_t)duty;
    uint32_t cmp = PWM_PERIOD - (PWM_PERIOD * abs_duty / 100);

    if (side == 0) {
        cc_index = DL_TIMER_CC_0_INDEX;
        if (duty < 0) {
            DL_TimerA_setCaptureCompareValue(PWM_Motor_INST, cmp, cc_index);
            DL_GPIO_setPins(GPIO_Motor_PIN_L1_PORT, GPIO_Motor_PIN_L1_PIN);
            DL_GPIO_clearPins(GPIO_Motor_PIN_L2_PORT, GPIO_Motor_PIN_L2_PIN);
        } else if (duty > 0) {
            DL_TimerA_setCaptureCompareValue(PWM_Motor_INST, cmp, cc_index);
            DL_GPIO_clearPins(GPIO_Motor_PIN_L1_PORT, GPIO_Motor_PIN_L1_PIN);
            DL_GPIO_setPins(GPIO_Motor_PIN_L2_PORT, GPIO_Motor_PIN_L2_PIN);
        } else {
            DL_GPIO_clearPins(GPIO_Motor_PIN_L1_PORT, GPIO_Motor_PIN_L1_PIN);
            DL_GPIO_clearPins(GPIO_Motor_PIN_L2_PORT, GPIO_Motor_PIN_L2_PIN);
        }
    } else {
        cc_index = DL_TIMER_CC_1_INDEX;
        if (duty < 0) {
            DL_TimerA_setCaptureCompareValue(PWM_Motor_INST, cmp, cc_index);
            DL_GPIO_setPins(GPIO_Motor_PIN_R1_PORT, GPIO_Motor_PIN_R1_PIN);
            DL_GPIO_clearPins(GPIO_Motor_PIN_R2_PORT, GPIO_Motor_PIN_R2_PIN);
        } else if (duty > 0) {
            DL_TimerA_setCaptureCompareValue(PWM_Motor_INST, cmp, cc_index);
            DL_GPIO_clearPins(GPIO_Motor_PIN_R1_PORT, GPIO_Motor_PIN_R1_PIN);
            DL_GPIO_setPins(GPIO_Motor_PIN_R2_PORT, GPIO_Motor_PIN_R2_PIN);
        } else {
            DL_GPIO_clearPins(GPIO_Motor_PIN_R1_PORT, GPIO_Motor_PIN_R1_PIN);
            DL_GPIO_clearPins(GPIO_Motor_PIN_R2_PORT, GPIO_Motor_PIN_R2_PIN);
        }
    }
}
