/**
 * @file    pid.h
 * @brief   位置式 PID + 串级位置环 —— 机试库
 *
 *  两个对象：
 *      速度环 PID_t spd         目标 RPM → 输出 u（-450~450）→ 电机
 *      位置环 PID_t pos         目标脉冲数 → 输出目标 RPM → 交给速度环
 *
 *  用法（速度环）：
 *      PID_Init(&spd, 0.5f, 0.03f, 0.0f, 450.0f, 3000.0f);
 *      float u = PID_Calc(&spd, 200.0f, rpm);
 *
 *  用法（位置环，串级）：
 *      float rpm_target = PID_Calc(&pos, target_pulse, now_pulse);
 *      if (rpm_target > 300) rpm_target = 300;      // 限速
 *      float u = PID_Calc(&spd, rpm_target, rpm);
 */
#ifndef __PID_H
#define __PID_H

#include <stdint.h>

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

/* 位置环：纯 P 就够用（外层建议 Kp=1~5, Ki=0, Kd=0） */
float PID_PositionCalc(PID_t *pos, PID_t *spd, float target_pos,
                       float now_pos, float now_rpm, float rpm_limit);

#endif /* __PID_H */
