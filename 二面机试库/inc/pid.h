/**
 * @file    pid.h
 * @brief   控制模块：PID 算法 + 编码器测速 + 目标波形
 *
 *  三部分放在一起，因为它们属于同一个「控制回路」：
 *      波形决定目标速度 → 编码器测出实际速度 → PID 算出输出
 *
 *  用法：
 *      // 开机一次
 *      ENCODER_Init();
 *      PID_Init(&g_spd, 0.5f, 0.03f, 0.0f, 450.0f, 3000.0f);
 *
 *      // 每 10ms
 *      g_rpm    = ENCODER_ReadRpm();
 *      g_target = WAVE_Get(g_mode, g_t);
 *      u        = PID_Calc(&g_spd, (float)g_target, g_rpm);
 */
#ifndef __PID_H
#define __PID_H

#include "main.h"      /* 编码器部分要用 TIM_HandleTypeDef */
#include <stdint.h>

/* ==================================================================
 * 一、PID 算法
 * ================================================================== */

typedef struct {
    float Kp, Ki, Kd;
    float e_last;      /* 上次误差（算 D） */
    float integral;    /* 误差累加（算 I） */
    float out_max;     /* 输出限幅 */
    float i_max;       /* 积分限幅，防积分饱和 */
} PID_t;

void  PID_Init (PID_t *p, float kp, float ki, float kd, float out_max, float i_max);
void  PID_Reset(PID_t *p);
float PID_Calc (PID_t *p, float target, float actual);

/* 串级位置环：位置环输出目标速度（限速后）→ 速度环输出 PWM */
float PID_PositionCalc(PID_t *pos, PID_t *spd, float target_pos,
                       float now_pos, float now_rpm, float rpm_limit);

/* ==================================================================
 * 二、编码器测速（TIM2 编码器模式）
 *
 *  CubeMX 配置：
 *      TIM2 → Combined Channels = Encoder Mode
 *             Prescaler = 0, Counter Period = 65535
 *             Encoder Mode = TI1 and TI2（×4 倍频）
 *      PA0 / PA1 → GPIO Pull-up（霍尔编码器多为开漏，必须上拉！）
 * ================================================================== */

#define ENC_PPR         1320.0f    /* 输出轴一圈的脉冲数（线数 x4 x 减速比） */
#define ENC_PERIOD_S    0.010f     /* 采样周期，秒。波形相位累加也用它，改一处即可 */
#define ENC_RPM_ALPHA   0.40f      /* 测速一阶低通，1.0 = 不滤波。一个脉冲约 4.5 rpm */

void    ENCODER_Init(void);
float   ENCODER_ReadRpm(void);      /* 读一次：更新 delta/position，返回 RPM */
int16_t ENCODER_GetDelta(void);     /* 上次的脉冲增量 */
int32_t ENCODER_GetPosition(void);  /* 累计脉冲（位置环用） */
void    ENCODER_ResetPosition(void);

/* ==================================================================
 * 三、目标波形（阶跃 / 固定正弦 / 变频正弦）
 * ================================================================== */

typedef enum {
    WAVE_STEP = 0,        /* 阶跃 */
    WAVE_SINE_FIX,        /* 固定频率 + 固定幅度正弦 */
    WAVE_SINE_SWEEP       /* 变频 + 变幅正弦 */
} WaveMode;

int32_t     WAVE_Get(WaveMode mode, float t);
const char *WAVE_Name(WaveMode mode);

#endif /* __PID_H */
