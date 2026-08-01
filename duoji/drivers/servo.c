/*
 * servo.c - Servo PWM Driver for MSPM0G3507
 *
 * TIMA0, edge-aligned up-counting PWM, PA8, 50Hz.
 */

#include "ti_msp_dl_config.h"
#include "servo.h"

/* ---- Track last-set angle for sweep ---- */
static uint8_t g_current_angle = SERVO_ANGLE_CENTER;

/*============================================================================
 * API
 *============================================================================*/

void servo_init(void)
{
    servo_set_angle(SERVO_ANGLE_CENTER);
}

void servo_set_angle(uint8_t angle_deg)
{
    uint32_t cc_value;

    if (angle_deg > SERVO_SAFE_MAX) angle_deg = SERVO_SAFE_MAX;
    if (angle_deg < SERVO_SAFE_MIN) angle_deg = SERVO_SAFE_MIN;

    g_current_angle = angle_deg;

    cc_value = ((uint32_t)angle_deg * SERVO_CC_SPAN) / SERVO_ANGLE_MAX
             + SERVO_CC_MIN;

    DL_TimerA_setCaptureCompareValue(SERVO_INST,
        (uint16_t)cc_value, DL_TIMER_CC_0_INDEX);
}

void servo_set_pulse_us(uint16_t pulse_us)
{
    if (pulse_us < SERVO_PULSE_MIN_US) pulse_us = SERVO_PULSE_MIN_US;
    if (pulse_us > SERVO_PULSE_MAX_US) pulse_us = SERVO_PULSE_MAX_US;

    DL_TimerA_setCaptureCompareValue(SERVO_INST,
        (uint16_t)((uint32_t)pulse_us * 2), DL_TIMER_CC_0_INDEX);
}

void servo_stop(void)
{
    servo_set_pulse_us(SERVO_PULSE_STOP_US);
}

/*
 * 360° continuous servo:
 *   +100 → 1000us (full forward)
 *      0 → 1500us (stop)
 *   -100 → 2000us (full reverse)
 */
void servo_set_speed(int8_t percent)
{
    int32_t pulse_us;

    if (percent > 100)  percent = 100;
    if (percent < -100) percent = -100;

    pulse_us = (int32_t)SERVO_PULSE_STOP_US - (int32_t)percent * 5;

    servo_set_pulse_us((uint16_t)pulse_us);
}

void servo_sweep(uint8_t target_deg, uint16_t step_ms)
{
    int16_t current = (int16_t)g_current_angle;
    int16_t target;

    if (target_deg > SERVO_SAFE_MAX) target_deg = SERVO_SAFE_MAX;
    if (target_deg < SERVO_SAFE_MIN) target_deg = SERVO_SAFE_MIN;

    target = (int16_t)target_deg;

    if (current < target) {
        for (; current <= target; current++) {
            servo_set_angle((uint8_t)current);
            if (step_ms > 0 && current < target)
                delay_cycles(CPUCLK_FREQ / 1000 * step_ms);
        }
    } else if (current > target) {
        for (; current >= target; current--) {
            servo_set_angle((uint8_t)current);
            if (step_ms > 0 && current > target)
                delay_cycles(CPUCLK_FREQ / 1000 * step_ms);
        }
    }
}
