#include "ti_msp_dl_config.h"
#include "grayscale_sensor.h"
#include "motor.h"
#include "PID.h"
#include "CY_Z.h"
#include "Encoder.h"


/*==================================================================
 *  模块内静态变量
 *==================================================================*/

/* PWM 输出一阶低通滤波状态（平滑电机响应） */
static float g_pwm_L_filt = 0.0f;
static float g_pwm_R_filt = 0.0f;

/* Control_Straight 丢线直行状态 */
static bool    g_straight_first = true;  // 首次进入标志
static float   g_yaw_angle      = 0.0f;  // 当前角度缓存
static uint8_t g_lost_count     = 0;     // 丢线直行累计次数

/* 角度参考数组：按键可切换 */
#define  ANGLE_COUNT   2
static float   g_profile_0[ANGLE_COUNT] = {0.0f, 180.0f};    // 按键1: 前/后
static float   g_profile_1[ANGLE_COUNT] = {38.66f, 141.34f};   // 按键2: 左/右
static float  *g_angle_refs = g_profile_0;                   // 当前使用的数组指针
static uint8_t g_angle_idx  = 0;

float Motor_Control_GetStraightRef(void)
{
    return g_angle_refs[g_angle_idx];
}

uint8_t Motor_Control_GetLostCount(void)
{
    return g_lost_count;
}

void Motor_Control_SetProfile(uint8_t n)
{
    g_angle_refs = n ? g_profile_1 : g_profile_0;
    g_angle_idx  = 0;
}

/*==================================================================
 *  控制 PID 实例（各函数共用）
 *==================================================================*/
static PID_TypeDef g_pid_track;     // 循迹转向 PID
static PID_TypeDef g_pid_yaw;       // 丢线直行角度 PID
static PID_TypeDef g_pid_spd_diff;  // 直行差速 PID（两轮转速差 → 0）


/*==================================================================
 * PID 调参区
 *==================================================================*/

 /*—————— 循迹 PID 调参区 ——————*/
#define TRACK_KP      0.88f    // 比例：快速响应偏移
#define TRACK_KI      0.02f   // 积分：弯道持续修正，消除稳态误差
#define TRACK_KD      12.5f    // 微分：抑制过冲，曲线更丝滑
#define TRACK_OUT_MAX 30.0f   // PID 输出上限（PWM 差速修正最大值）

/*—————— 直行 PID 调参区 ——————*/
#define STRAIGHT_DIFF_KP  3.0f    // 差速环比例（两轮转速差 → 0）
#define STRAIGHT_DIFF_KI  0.05f   // 差速环积分
#define STRAIGHT_DIFF_KD  5.0f    // 差速环微分
#define STRAIGHT_DIFF_MAX 15.0f   // 差速环输出上限

/*—————— 角度 PID 调参区 ——————*/
#define STRAIGHT_YAW_KP   1.6f    // 角度环比例（CY-Z 航向保持）
#define STRAIGHT_YAW_KI   0.03f   // 角度环积分
#define STRAIGHT_YAW_KD   15.0f    // 角度环微分
#define STRAIGHT_YAW_MAX  25.0f   // 角度环输出上限


/*==================================================================
 *  Motor_Control_Init — 初始化所有控制 PID
 *==================================================================*/
void Motor_Control_Init(void)
{
    PID_Init(&g_pid_track,
        TRACK_KP, TRACK_KI, TRACK_KD,
        TRACK_OUT_MAX, -TRACK_OUT_MAX);

    PID_Init(&g_pid_yaw,
        STRAIGHT_YAW_KP, STRAIGHT_YAW_KI, STRAIGHT_YAW_KD,
        STRAIGHT_YAW_MAX, -STRAIGHT_YAW_MAX);

    PID_Init(&g_pid_spd_diff,
        STRAIGHT_DIFF_KP, STRAIGHT_DIFF_KI, STRAIGHT_DIFF_KD,
        STRAIGHT_DIFF_MAX, -STRAIGHT_DIFF_MAX);
}


/*==================================================================
 *  Control_Line_Track — 循线行驶：循迹环
 *==================================================================*/
void Control_Line_Track(void)
{
    float error;
    float turn_adj;
    float target_L, target_R;
    int8_t pwm_L, pwm_R;

    /* 刚从丢线恢复到循线 → 推进角度索引，下次丢线用下一个参考 */
    if (!g_straight_first) {
        g_angle_idx = (g_angle_idx + 1) % ANGLE_COUNT;
    }
    g_straight_first = true;

    /*—————— 循迹环：传感器误差 → 转向 PID ——————*/
    error = (float)(cx - LINE_CENTER);          // cx 偏离中心 (35) 的量

    /* 死区：微小偏差不响应，抑制中心附近抖动 */
    if (error > -(float)DEAD_ZONE && error < (float)DEAD_ZONE) {
        error = 0.0f;
    }

    turn_adj = PID_Update(&g_pid_track, error); // PID 计算差速修正量

    /*—————— 速度环：基础 PWM ± 转向修正 ——————
     * error > 0 (线偏左) → turn_adj > 0 → 左+右- → 车右转追线
     * 当前为开环速度；后续接入编码器后可替换为 PID 速度闭环 */
    target_L = (float)BASE_PWM + turn_adj;
    target_R = (float)BASE_PWM - turn_adj;

    /* 一阶低通滤波 — 平滑 PWM 变化，过弯更丝滑 */
    g_pwm_L_filt += FILTER_ALPHA * (target_L - g_pwm_L_filt);
    g_pwm_R_filt += FILTER_ALPHA * (target_R - g_pwm_R_filt);

    /* 限幅并转整数 */
    pwm_L = (int8_t)g_pwm_L_filt;
    pwm_R = (int8_t)g_pwm_R_filt;

    if (pwm_L > MAX_PWM) { pwm_L = MAX_PWM; g_pwm_L_filt = MAX_PWM; }
    if (pwm_L < MIN_PWM) { pwm_L = MIN_PWM; g_pwm_L_filt = MIN_PWM; }
    if (pwm_R > MAX_PWM) { pwm_R = MAX_PWM; g_pwm_R_filt = MAX_PWM; }
    if (pwm_R < MIN_PWM) { pwm_R = MIN_PWM; g_pwm_R_filt = MIN_PWM; }

    /*—————— 输出到电机 ——————*/
    Motor_On();
    Set_Speed(0, pwm_L);    // 左轮
    Set_Speed(1, pwm_R);    // 右轮
}

/*==================================================================
 *  Control_Straight — 直行：速度环 + 角度环
 *==================================================================*/

void Control_Straight(void)
{
    CY_Z_Telemetry telem;
    float yaw_error, yaw_corr;
    float spd_diff, spd_corr;
    float target_L, target_R;
    int8_t pwm_L, pwm_R;

    /*—————— 获取传感器数据 ——————*/
    if (CY_Z_GetTelemetry(&telem)) {
        g_yaw_angle = telem.angle_deg;
    }

    /*—————— 首次进入：计数 + 复位 PID ——————*/
    if (g_straight_first) {
        g_lost_count++;
        PID_Reset(&g_pid_yaw);
        PID_Reset(&g_pid_spd_diff);
        g_straight_first = false;
    }

    /*—————— 角度环：维持航向 = 当前参考角度 ——————
     * 归一化到 [-180,180]，确保走最短路径（解决 180° 绕远问题）
     * 车头左偏(+) → 需右转 → 左+ 右- */
    yaw_error = g_yaw_angle - g_angle_refs[g_angle_idx];
    while (yaw_error > 180.0f)  yaw_error -= 360.0f;
    while (yaw_error < -180.0f) yaw_error += 360.0f;
    yaw_corr  = PID_Update(&g_pid_yaw, yaw_error);

    /*—————— 速度环：编码器差速 → 0 ——————
     * spd_diff > 0 → 左轮偏快 → spd_corr > 0 → 左- 右+ → 两轮等速 */
    spd_diff = (float)Encoder_GetSpeedL() - (float)Encoder_GetSpeedR();
    spd_corr = PID_Update(&g_pid_spd_diff, spd_diff);

    /* 差速修正 + 角度修正 叠加到基础 PWM */
    target_L = (float)BASE_PWM - spd_corr + yaw_corr;
    target_R = (float)BASE_PWM + spd_corr - yaw_corr;

    /* 一阶低通滤波 */
    g_pwm_L_filt += FILTER_ALPHA * (target_L - g_pwm_L_filt);
    g_pwm_R_filt += FILTER_ALPHA * (target_R - g_pwm_R_filt);

    /* 限幅 */
    pwm_L = (int8_t)g_pwm_L_filt;
    pwm_R = (int8_t)g_pwm_R_filt;

    if (pwm_L > MAX_PWM) { pwm_L = MAX_PWM; g_pwm_L_filt = MAX_PWM; }
    if (pwm_L < MIN_PWM) { pwm_L = MIN_PWM; g_pwm_L_filt = MIN_PWM; }
    if (pwm_R > MAX_PWM) { pwm_R = MAX_PWM; g_pwm_R_filt = MAX_PWM; }
    if (pwm_R < MIN_PWM) { pwm_R = MIN_PWM; g_pwm_R_filt = MIN_PWM; }

    /*—————— 输出到电机 ——————*/
    Motor_On();
    Set_Speed(0, pwm_L);
    Set_Speed(1, pwm_R);
}


/*==================================================================
 *  Control_Corner — 直角转弯
 *==================================================================*/

/*—————— 转弯调参区 ——————*/
#define CORNER_FAST      35      // 外侧轮 PWM (%)
#define CORNER_SLOW      15       // 内侧轮 PWM (%)

void Control_Corner(int8_t dir)
{
    Motor_On();

    if (dir > 0) {
        /* 右转：左快 + 右慢 */
        Set_Speed(0,  CORNER_FAST);
        Set_Speed(1,  CORNER_SLOW);
    } else {
        /* 左转：左慢 + 右快 */
        Set_Speed(0,  CORNER_SLOW);
        Set_Speed(1,  CORNER_FAST);
    }

    g_pwm_L_filt = 0.0f;
    g_pwm_R_filt = 0.0f;
}
