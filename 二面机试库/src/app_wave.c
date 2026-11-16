/**
 * @file    app_wave.c
 * @brief   三种目标波形生成
 *
 *  统一约定：目标转速一律 >= 0（速度环里不生成负目标，负速度会撞死区 + 换向冲击）
 *
 *  变频正弦为什么用"相位累加"而不是 sin(2π f t)（面试加分点）：
 *      频率 f 随时间变化时，2π·f(t)·t 的值会突变，波形会断掉。
 *      相位累加只加"这一小步的角度增量"，波形天然连续。
 *      这是 DDS（直接数字频率合成）的标准做法。
 */
#include "app_wave.h"
#include <math.h>

#define PI2   6.2831853f

/* 波形参数（按需要改） */
#define WAVE_CENTER       200.0f   /* 中心转速 */
#define WAVE_AMP_FIX       50.0f   /* 固定正弦幅度 */
#define WAVE_FREQ_FIX       0.5f   /* 固定正弦频率 Hz */

#define WAVE_SWEEP_PERIOD  12.0f   /* 扫频一轮的时长，秒 */
#define WAVE_FREQ_MIN       0.2f
#define WAVE_FREQ_MAX       2.6f
#define WAVE_AMP_MIN       30.0f
#define WAVE_AMP_MAX       80.0f

/* ① 阶跃：每 4 秒换一档 */
static int32_t wave_step(float t)
{
    if (t < 4.0f)   return 100;
    if (t < 8.0f)   return 200;
    if (t < 12.0f)  return 300;
    if (t < 16.0f)  return 100;
    return 100;
}

/* ② 固定频率 + 固定幅度 */
static int32_t wave_sine_fix(float t)
{
    float v = WAVE_CENTER + WAVE_AMP_FIX * sinf(PI2 * WAVE_FREQ_FIX * t);
    if (v < 0.0f) v = 0.0f;
    return (int32_t)v;
}

/* ③ 变频 + 变幅：相位累加 */
static int32_t wave_sine_sweep(float t)
{
    static float phase = 0.0f;
    static float t_prev = 0.0f;

    /* 时间被外部重置时同步清相位 */
    if (t < t_prev)
        phase = 0.0f;
    t_prev = t;

    /* 用取余实现"每 PERIOD 秒扫一轮" */
    float p  = fmodf(t, WAVE_SWEEP_PERIOD) / WAVE_SWEEP_PERIOD;   /* 0~1 */
    float f  = WAVE_FREQ_MIN + (WAVE_FREQ_MAX - WAVE_FREQ_MIN) * p;
    float amp = WAVE_AMP_MIN + (WAVE_AMP_MAX - WAVE_AMP_MIN) * p;

    /* 关键：累加相位增量，而不是直接算绝对相位 */
    phase += PI2 * f * 0.01f;                 /* 0.01 = 控制周期 10ms */
    if (phase > PI2 * 1000.0f)
        phase -= PI2 * 1000.0f;               /* 防 float 精度损失 */

    float v = WAVE_CENTER + amp * sinf(phase);
    if (v < 0.0f) v = 0.0f;
    return (int32_t)v;
}

int32_t APP_Wave_Get(WaveMode mode, float t)
{
    switch (mode)
    {
        case WAVE_STEP:       return wave_step(t);
        case WAVE_SINE_FIX:   return wave_sine_fix(t);
        case WAVE_SINE_SWEEP: return wave_sine_sweep(t);
        default:              return (int32_t)WAVE_CENTER;
    }
}

const char *APP_Wave_Name(WaveMode mode)
{
    switch (mode)
    {
        case WAVE_STEP:       return "STEP";
        case WAVE_SINE_FIX:   return "SINE_FIX";
        case WAVE_SINE_SWEEP: return "SINE_SWEEP";
        default:              return "?";
    }
}
