/**
 * @file    pid.c
 * @brief   位置式 PID 实现
 *
 *  公式：u = Kp*e + Ki*Σe + Kd*(e - e_last)
 *
 *  为什么必须传指针（面试会问）：
 *      integral 和 e_last 要跨调用保留。传值的话函数改的是副本，
 *      每次返回就丢，I 项永远退化成"当前误差"，消除不了稳态误差。
 *
 *  两个限幅都必须有：
 *      i_max   防积分饱和（堵转/大阶跃时 integral 会跑到几万）
 *      out_max 防输出越界（PWM 的 CCR 有上限）
 */
#include "pid.h"

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

    /* ---- 积分累加 + 积分限幅 ---- */
    p->integral += e;
    if (p->integral >  p->i_max) p->integral =  p->i_max;
    if (p->integral < -p->i_max) p->integral = -p->i_max;

    /* 存下本次误差，注意必须在算完 d 之后 */
    p->e_last = e;

    /* ---- 三项相加 ---- */
    out = p->Kp * e
        + p->Ki * p->integral
        + p->Kd * d;

    /* ---- 输出限幅 ---- */
    if (out >  p->out_max) out =  p->out_max;
    if (out < -p->out_max) out = -p->out_max;

    return out;
}

/**
 * 串级位置环：
 *      外层（位置）算出"该用多大速度冲过去"，内层（速度）负责稳稳跟上。
 *      外层输出一定要限速，否则位置误差大时会给一个不可能达到的转速。
 */
float PID_PositionCalc(PID_t *pos, PID_t *spd, float target_pos,
                       float now_pos, float now_rpm, float rpm_limit)
{
    float rpm_target;

    rpm_target = PID_Calc(pos, target_pos, now_pos);

    if (rpm_target >  rpm_limit) rpm_target =  rpm_limit;
    if (rpm_target < -rpm_limit) rpm_target = -rpm_limit;

    return PID_Calc(spd, rpm_target, now_rpm);
}
