/**
 * @file    bsp_motor.c
 * @brief   电机 PWM 控制：正反转 + 死区补偿 + 输出限幅
 *
 *  三个要点（面试会问）：
 *  ① 方向由"哪个通道出占空比"决定，不是往寄存器塞负数
 *     CCR 是 16 位无符号寄存器，塞 -500 会变成 65036 > ARR → 100% 占空比
 *  ② 两个通道每次都要写，没写的那个会保持上一次的值 → H 桥两输入同时有效 → 刹车
 *  ③ 死区补偿：ccr = DEAD + |u|，把 PID 输出的原点挪到电机真正开始转的位置
 */
#include "bsp_motor.h"

extern TIM_HandleTypeDef htim3;

void BSP_Motor_Init(void)
{
    HAL_TIM_PWM_Start(&htim3, MOTOR_CH_FWD);
    HAL_TIM_PWM_Start(&htim3, MOTOR_CH_REV);
    BSP_Motor_Stop();
}

void BSP_Motor_Stop(void)
{
    __HAL_TIM_SET_COMPARE(&htim3, MOTOR_CH_FWD, 0);
    __HAL_TIM_SET_COMPARE(&htim3, MOTOR_CH_REV, 0);
}

void BSP_Motor_Set(int16_t u)
{
    int32_t ccr;

    /* 输出限幅：PID 可能算出超出范围的值 */
    if (u > MOTOR_U_MAX)
        u = MOTOR_U_MAX;
    if (u < -MOTOR_U_MAX)
        u = -MOTOR_U_MAX;

    /* 死区补偿 + 取绝对值（负号已经通过"用哪个通道"表达掉了） */
    ccr = MOTOR_PWM_DEAD + (u >= 0 ? u : -u);
    if (ccr > MOTOR_PWM_LIMIT)
        ccr = MOTOR_PWM_LIMIT;

    if (u > 0) /* 正转 */
    {
        __HAL_TIM_SET_COMPARE(&htim3, MOTOR_CH_FWD, ccr);
        __HAL_TIM_SET_COMPARE(&htim3, MOTOR_CH_REV, 0);
    }
    else if (u < 0) /* 反转 */
    {
        __HAL_TIM_SET_COMPARE(&htim3, MOTOR_CH_FWD, 0);
        __HAL_TIM_SET_COMPARE(&htim3, MOTOR_CH_REV, ccr);
    }
    else /* 停：两路都清零 */
    {
        BSP_Motor_Stop();
    }
}
