/**
 * @file    bsp_motor.h
 * @brief   电机 PWM 控制（TB6612 / AIN1+AIN2 两路 PWM 接法）—— 机试库
 *
 *  接线前提（与任务 3.2 / 任务 6 一致）：
 *      PA6 / TIM3_CH1 → AIN1
 *      PA7 / TIM3_CH2 → AIN2
 *      PA3            → STBY（高电平使能）
 *      TB6612 的 PWMA 必须接高电平（3.3V/5V），否则无输出
 *
 *  用法：
 *      BSP_Motor_Init();
 *      BSP_Motor_Set(200);      // 正转 200（-450 ~ +450）
 *      BSP_Motor_Set(-200);     // 反转
 *      BSP_Motor_Stop();        // 停
 */
#ifndef __BSP_MOTOR_H
#define __BSP_MOTOR_H

#include "main.h"
#include <stdint.h>

/* ===== 这两个宏按实际接线改（若换定时器/通道） ===== */
#define MOTOR_HANDLE     htim3
#define MOTOR_CH_FWD     TIM_CHANNEL_1     /* 正转 → AIN1 */
#define MOTOR_CH_REV     TIM_CHANNEL_2     /* 反转 → AIN2 */

/* ===== 死区与限幅：按实测死区修改 ===== */
#define MOTOR_PWM_DEAD   500      /* CCR 低于此值电机不转（死区补偿基准） */
#define MOTOR_PWM_LIMIT  950      /* 最大 CCR（占空比上限） */
#define MOTOR_U_MAX      450      /* PID 输出限幅 = LIMIT - DEAD */

void BSP_Motor_Init(void);
void BSP_Motor_Stop(void);
void BSP_Motor_Set(int16_t u);     /* u: -450 ~ +450，正负表示方向 */

#endif /* __BSP_MOTOR_H */
