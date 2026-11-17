/**
 * @file    pid.c
 * @brief   控制模块：PID 算法 + 编码器测速 + 目标波形
 */
#include "pid.h"
#include <math.h>

/* ==================================================================
 * 一、PID 算法
 *
 *  公式：u = Kp*e + Ki*Σe + Kd*(e - e_last)
 *
 *  为什么必须传指针：
 *      integral 和 e_last 要跨调用保留。传值改的是副本，
 *      每次返回就丢，I 项永远退化成"当前误差"，消不掉稳态误差。
 *
 *  两个限幅都必须有：
 *      i_max   防积分饱和（堵转/大阶跃时 integral 能跑到几万）
 *      out_max 防输出越界（PWM 的 CCR 有上限）
 * ================================================================== */

void PID_Init(PID_t *p, float kp, float ki, float kd, float out_max, float i_max)
{
    p->Kp       = kp;
    p->Ki       = ki;
    p->Kd       = kd;
    p->out_max  = out_max;
    p->i_max    = i_max;
    p->e_last   = 0.0f;
    p->integral = 0.0f;
}

void PID_Reset(PID_t *p)
{
    p->e_last   = 0.0f;
    p->integral = 0.0f;
}

float PID_Calc(PID_t *p, float target, float actual)
{
    float e  = target - actual;      /* 误差 */
    float d  = e - p->e_last;        /* 误差变化量（给 D） */
    float out;

    /* 积分累加 + 积分限幅 */
    p->integral += e;
    if (p->integral >  p->i_max) p->integral =  p->i_max;
    if (p->integral < -p->i_max) p->integral = -p->i_max;

    /* 存本次误差，必须在算完 d 之后 */
    p->e_last = e;

    /* 三项相加 */
    out = p->Kp * e
        + p->Ki * p->integral
        + p->Kd * d;

    /* 输出限幅 */
    if (out >  p->out_max) out =  p->out_max;
    if (out < -p->out_max) out = -p->out_max;

    return out;
}

float PID_PositionCalc(PID_t *pos, PID_t *spd, float target_pos,
                       float now_pos, float now_rpm, float rpm_limit)
{
    float rpm_target;

    /* 外层：位置误差 -> 该用多快冲过去 */
    rpm_target = PID_Calc(pos, target_pos, now_pos);

    /* 一定要限速，否则位置误差大时会给出不可能达到的转速 */
    if (rpm_target >  rpm_limit) rpm_target =  rpm_limit;
    if (rpm_target < -rpm_limit) rpm_target = -rpm_limit;

    /* 内层：速度环输出 PWM */
    return PID_Calc(spd, rpm_target, now_rpm);
}

/* ==================================================================
 * 二、编码器测速
 *
 *  为什么必须 (int16_t) 强转：
 *      TIM2 的 CNT 是 16 位，从 65535 溢出回 0。
 *      直接相减：5 - 65530 = -65525（错）
 *      强转后按补码解释：(int16_t)5 - (int16_t)65530 = 5 - (-6) = 11（对）
 * ================================================================== */

extern TIM_HandleTypeDef htim2;

static int16_t s_last_cnt = 0;
static int16_t s_delta    = 0;
static int32_t s_position = 0;
static float   s_rpm_filt = 0.0f;   /* 低通后的转速，给 PID 用 */

void ENCODER_Init(void)
{
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);   /* 必须 ALL，不是 CH1 */
    s_last_cnt = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
    s_delta    = 0;
    s_position = 0;
    s_rpm_filt = 0.0f;
}

float ENCODER_ReadRpm(void)
{
    int16_t now = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);

    s_delta    = now - s_last_cnt;    /* 这 10ms 数到几个脉冲 */
    s_last_cnt = now;                 /* 记住，下次要用 */
    s_position += s_delta;            /* 累计位置 */

    /* RPM = (delta / PPR) / T × 60 = delta × 60 / (PPR × T)
     * 1320 脉冲、10ms 时，一个脉冲就是约 4.5 rpm，原始值是台阶。
     * 一阶低通：y += α*(x-y)。α=1 等于不滤。滞后大约 (1-α)/α 个周期。 */
    float rpm = (float)s_delta * 60.0f / (ENC_PPR * ENC_PERIOD_S);
    s_rpm_filt += ENC_RPM_ALPHA * (rpm - s_rpm_filt);
    return s_rpm_filt;
}

int16_t ENCODER_GetDelta(void)      { return s_delta; }
int32_t ENCODER_GetPosition(void)   { return s_position; }
void    ENCODER_ResetPosition(void) { s_position = 0; }

/* ==================================================================
 * 三、目标波形
 *
 *  统一约定：目标转速一律 >= 0（速度环里不生成负目标，负速度会撞死区）
 *
 *  变频正弦为什么用「相位累加」而不是 sin(2π f t)：
 *      频率 f 随时间变化时，2π·f(t)·t 的值会突变，波形会断掉。
 *      相位累加只加"这一小步的角度增量"，波形天然连续（DDS 做法）。
 * ================================================================== */

#define PI2   6.2831853f

/* 波形参数（按需要改） */
#define WAVE_CENTER       200.0f
#define WAVE_AMP_FIX       50.0f
#define WAVE_FREQ_FIX       0.5f

#define WAVE_SWEEP_PERIOD  12.0f
#define WAVE_FREQ_MIN       0.2f
#define WAVE_FREQ_MAX       2.6f
#define WAVE_AMP_MIN       30.0f
#define WAVE_AMP_MAX       80.0f

/* 阶跃：100 -> 200 -> 300 -> 100，每段 4 秒，16 秒一轮 */
static int32_t wave_step(float t)
{
    float s = fmodf(t, 16.0f);          /* 切到 0~16 秒，循环 */

    if      (s <  4.0f) return 100;
    else if (s <  8.0f) return 200;
    else if (s < 12.0f) return 300;
    else                return 100;
}

/* 固定频率 + 固定幅度 */
static int32_t wave_sine_fix(float t)
{
    float v = WAVE_CENTER + WAVE_AMP_FIX * sinf(PI2 * WAVE_FREQ_FIX * t);
    if (v < 0.0f) v = 0.0f;
    return (int32_t)v;
}

/* 变频 + 变幅：相位累加 */
static int32_t wave_sine_sweep(float t)
{
    static float phase  = 0.0f;
    static float t_prev = 0.0f;

    if (t < t_prev) phase = 0.0f;       /* 时间被重置 -> 相位清零 */
    t_prev = t;

    /* 每 WAVE_SWEEP_PERIOD 秒扫一轮 */
    float p = fmodf(t, WAVE_SWEEP_PERIOD) / WAVE_SWEEP_PERIOD;   /* 0~1 */
    float f = WAVE_FREQ_MIN + (WAVE_FREQ_MAX - WAVE_FREQ_MIN) * p;
    float a = WAVE_AMP_MIN  + (WAVE_AMP_MAX  - WAVE_AMP_MIN)  * p;

    /* 关键：只加"这一小步"的角度，不重算绝对相位 */
    phase += PI2 * f * ENC_PERIOD_S;    /* 和测速用同一个周期，改一处即可 */
    if (phase > PI2) phase -= PI2;

    float v = WAVE_CENTER + a * sinf(phase);
    if (v < 0.0f) v = 0.0f;
    return (int32_t)v;
}

int32_t WAVE_Get(WaveMode mode, float t)
{
    switch (mode)
    {
        case WAVE_STEP:       return wave_step(t);
        case WAVE_SINE_FIX:   return wave_sine_fix(t);
        case WAVE_SINE_SWEEP: return wave_sine_sweep(t);
        default:              return (int32_t)WAVE_CENTER;
    }
}

const char *WAVE_Name(WaveMode mode)
{
    switch (mode)
    {
        case WAVE_STEP:       return "STEP";
        case WAVE_SINE_FIX:   return "SINEFIX";
        case WAVE_SINE_SWEEP: return "SINSWP";
        default:              return "?";
    }
}
