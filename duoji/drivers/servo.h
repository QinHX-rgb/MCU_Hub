/*
 * servo.h - Servo PWM Driver for MSPM0G3507
 *
 * Drives an RC servo via TIMA0 PWM on PA8 (configured in SysConfig).
 * PWM: 50Hz (20ms frame), timer clock 2MHz.
 *
 * Hardware-safe angle range: 10° ~ 140°
 *
 *   0.5ms (CC=1000) →   0°
 *   1.5ms (CC=3000) →  90° (center)
 *   2.5ms (CC=5000) → 180°
 */

#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

/* --- Angle range --- */
#define SERVO_ANGLE_MIN     0
#define SERVO_ANGLE_MAX     180
#define SERVO_ANGLE_CENTER  90

/* --- Hardware-safe limits (do not exceed!) --- */
#define SERVO_SAFE_MIN      10
#define SERVO_SAFE_MAX      140

/* --- Pulse width (microseconds) --- */
#define SERVO_PULSE_MIN_US    500
#define SERVO_PULSE_MAX_US    2500
#define SERVO_PULSE_STOP_US   1500

/* --- Compare-Capture values (timer clock = 2MHz → 1us = 2 ticks) --- */
#define SERVO_CC_MIN    1000
#define SERVO_CC_MAX    5000
#define SERVO_CC_STOP   3000
#define SERVO_CC_SPAN   (SERVO_CC_MAX - SERVO_CC_MIN)

/*==================== API ====================*/

void servo_init(void);
void servo_set_angle(uint8_t angle_deg);       /* clamped to [45°, 140°] */
void servo_set_pulse_us(uint16_t pulse_us);    /* clamped to [500, 2500] us */
void servo_stop(void);                         /* 360° servo: stop at 1500us */
void servo_set_speed(int8_t percent);          /* 360° servo: -100 ~ +100 */
void servo_sweep(uint8_t target_deg, uint16_t step_ms);

#endif
