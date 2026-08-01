#include "ti_msp_dl_config.h"
#include "app_config.h"
#include "motor.h"
#include "Motor_Control.h"
#include "clock.h"

static float g_pwm_L_filt = 0.0f;
static float g_pwm_R_filt = 0.0f;
static float g_line_pos_filt = (float)LINE_CENTER;
static float g_last_error = 0.0f;
static float g_error_rate_filt = 0.0f;
static uint8_t g_base_pwm = BASE_PWM_FAST;
static unsigned long g_soft_start_ms = 0;
static unsigned long g_decel_start_ms = 0;

int16_t g_dbg_L = 0;
int16_t g_dbg_R = 0;

static float AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float ClampFloat(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static float ApplyContinuousDeadZone(float error)
{
    if (error > LINE_DEAD_ZONE) {
        return error - LINE_DEAD_ZONE;
    }
    if (error < -LINE_DEAD_ZONE) {
        return error + LINE_DEAD_ZONE;
    }
    return 0.0f;
}

void Motor_Init(void)
{
    Motor_ResetControl();
    Motor_Off();
}

void Motor_ResetControl(void)
{
    g_pwm_L_filt = 0.0f;
    g_pwm_R_filt = 0.0f;
    g_line_pos_filt = (float)LINE_CENTER;
    g_last_error = 0.0f;
    g_error_rate_filt = 0.0f;
    g_base_pwm = BASE_PWM_FAST;
    g_soft_start_ms = 0;
    g_decel_start_ms = 0;
}

void Motor_SetSpeedFast(bool fast)
{
    g_base_pwm = fast ? BASE_PWM_FAST : BASE_PWM_SLOW;
    g_soft_start_ms = tick_ms;
}

void Motor_StartDeceleration(void)
{
    if (g_decel_start_ms == 0) {
        g_decel_start_ms = tick_ms;
    }
}

void Motor_SetDifferential(int8_t left, int8_t right)
{
    if (left > MAX_PWM) {
        left = MAX_PWM;
    } else if (left < MIN_PWM) {
        left = MIN_PWM;
    }

    if (right > MAX_PWM) {
        right = MAX_PWM;
    } else if (right < MIN_PWM) {
        right = MIN_PWM;
    }

    g_dbg_L = left;
    g_dbg_R = right;
    Motor_On();
    Set_Speed(0, left);
    Set_Speed(1, right);
}

void Motor_FollowLine(int16_t position)
{
    float error;
    float error_rate;
    float turn;
    float base_pwm;
    float target_left;
    float target_right;
    int8_t pwm_left;
    int8_t pwm_right;

    /*
     * 先平滑离散的 8 路重心，再对误差变化率单独低通。
     * 两级滤波可抑制单通道跳变引起的左右来回修正。
     */
    g_line_pos_filt +=
        LINE_FILTER_ALPHA * ((float)position - g_line_pos_filt);
    error = ApplyContinuousDeadZone(
        g_line_pos_filt - (float)LINE_CENTER);

    error_rate = error - g_last_error;
    g_error_rate_filt += DERIVATIVE_FILTER_ALPHA *
        (error_rate - g_error_rate_filt);
    g_last_error = error;

    turn = STEER_KP * error + STEER_KD * g_error_rate_filt;
    turn = ClampFloat(turn, -STEER_LIMIT, STEER_LIMIT);

    /*
     * 慢速档软启动：从 SLOW_SOFT_START_MIN_PWM 线性加速到目标速度，
     * 持续 SLOW_SOFT_START_MS。快速档不做软启动。
     */
    {
        float effective_base = (float)g_base_pwm;
        float min_pwm = CURVE_MIN_PWM;

        if (g_soft_start_ms != 0 && g_base_pwm == BASE_PWM_SLOW) {
            unsigned long elapsed = tick_ms - g_soft_start_ms;
            if (elapsed < SLOW_SOFT_START_MS) {
                float ratio = (float)elapsed / (float)SLOW_SOFT_START_MS;
                effective_base = SLOW_SOFT_START_MIN_PWM +
                    ((float)g_base_pwm - SLOW_SOFT_START_MIN_PWM) * ratio;
                /* 软启动期间同步抬高最低 PWM 限制，允许从 0 起步 */
                if (effective_base < CURVE_MIN_PWM) {
                    min_pwm = effective_base;
                }
            }
        }

        /* 快速档变速：15 秒后基础速度从 38% 降至 30% */
        if (g_base_pwm == BASE_PWM_FAST && g_soft_start_ms != 0) {
            unsigned long elapsed = tick_ms - g_soft_start_ms;
            if (elapsed >= FAST_SPEED_SWITCH_MS) {
                effective_base = (float)FAST_PWM_AFTER;
            }
        }

        /* 23 秒定时减速：SLOW_DECEL_MS 内线性降到 0 */
        if (g_decel_start_ms != 0) {
            unsigned long decel_elapsed = tick_ms - g_decel_start_ms;
            if (decel_elapsed < SLOW_DECEL_MS) {
                float ratio = (float)decel_elapsed / (float)SLOW_DECEL_MS;
                effective_base = effective_base * (1.0f - ratio);
                if (effective_base < CURVE_MIN_PWM) {
                    min_pwm = effective_base;
                }
            } else {
                effective_base = 0.0f;
                min_pwm = 0.0f;
            }
        }

        /*
         * 偏差越大越主动降速，使半径 0.5 m 的连续弧线有充足修正时间；
         * 居中后恢复基础速度，避免用过大的差速反复越过黑线。
         */
        base_pwm = effective_base -
            CURVE_SLOWDOWN * (AbsFloat(turn) / STEER_LIMIT);
        if (base_pwm < min_pwm) {
            base_pwm = min_pwm;
        }
    }

    target_left = base_pwm + turn;
    target_right = base_pwm - turn;

    g_pwm_L_filt +=
        PWM_FILTER_ALPHA * (target_left - g_pwm_L_filt);
    g_pwm_R_filt +=
        PWM_FILTER_ALPHA * (target_right - g_pwm_R_filt);

    pwm_left = (int8_t)(g_pwm_L_filt + 0.5f);
    pwm_right = (int8_t)(g_pwm_R_filt + 0.5f);
    Motor_SetDifferential(pwm_left, pwm_right);
}
