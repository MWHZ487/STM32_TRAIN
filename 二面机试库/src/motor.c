/**
 * @file    motor.c
 * @brief   电机 PWM 控制：正反转 + 死区补偿 + 输出限幅
 *
 *  三个要点（面试会问）：
 *  ① 方向由"哪个通道出占空比"决定，不是往寄存器塞负数
 *     CCR 是 16 位无符号寄存器，塞 -500 会变成 65036 > ARR → 100% 占空比
 *  ② 两个通道每次都要写，没写的那个会保持上一次的值 → H 桥两输入同时有效 → 刹车
 *  ③ 死区补偿：ccr = DEAD + |u|，把 PID 输出的原点挪到电机真正开始转的位置
 */
#include "motor.h"

extern TIM_HandleTypeDef MOTOR_HANDLE;   /* 展开成 htim3，换定时器只改 motor.h */

void MOTOR_Init(void)
{
    HAL_TIM_PWM_Start(&MOTOR_HANDLE, MOTOR_CH_FWD);
    HAL_TIM_PWM_Start(&MOTOR_HANDLE, MOTOR_CH_REV);
    HAL_GPIO_WritePin(MOTOR_STBY_PORT, MOTOR_STBY_PIN, GPIO_PIN_SET);
    MOTOR_Stop();
}

void MOTOR_Stop(void)
{
    __HAL_TIM_SET_COMPARE(&MOTOR_HANDLE, MOTOR_CH_FWD, 0);
    __HAL_TIM_SET_COMPARE(&MOTOR_HANDLE, MOTOR_CH_REV, 0);
}

void MOTOR_Set(int16_t u)
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

    /* 先清另一路再写本路。顺序反了的话，换向的一瞬间两路同时有效，H 桥刹车 */
    if (u > 0) /* 正转 */
    {
        __HAL_TIM_SET_COMPARE(&MOTOR_HANDLE, MOTOR_CH_REV, 0);
        __HAL_TIM_SET_COMPARE(&MOTOR_HANDLE, MOTOR_CH_FWD, ccr);
    }
    else if (u < 0) /* 反转 */
    {
        __HAL_TIM_SET_COMPARE(&MOTOR_HANDLE, MOTOR_CH_FWD, 0);
        __HAL_TIM_SET_COMPARE(&MOTOR_HANDLE, MOTOR_CH_REV, ccr);
    }
    else /* 停：两路都清零 */
    {
        MOTOR_Stop();
    }
}
