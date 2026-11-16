/**
 * @file    bsp_encoder.h
 * @brief   编码器测速 / 位置累计 —— 机试库
 *
 *  CubeMX 配置：
 *      TIM2 → Combined Channels = Encoder Mode
 *             Prescaler = 0, Counter Period = 65535
 *             Encoder Mode = TI1 and TI2（×4 倍频）
 *      PA0 / PA1 → GPIO Pull-up（霍尔编码器多为开漏输出，必须上拉！）
 *
 *  用法（在 10ms 定时中断里）：
 *      float rpm = BSP_Encoder_ReadRpm();      // 顺便更新 delta 和 position
 *      int32_t pos = BSP_Encoder_GetPosition();
 */
#ifndef __BSP_ENCODER_H
#define __BSP_ENCODER_H

#include "main.h"
#include <stdint.h>

/* ===== 按实测修改 ===== */
#define ENC_PPR         1320.0f   /* 输出轴一圈的脉冲数（线数 × 4 × 减速比） */
#define ENC_PERIOD_S    0.010f    /* 采样周期，秒。10ms → 0.010 */

void    BSP_Encoder_Init(void);
float   BSP_Encoder_ReadRpm(void);     /* 读一次：更新 delta/position，返回 RPM */
int16_t BSP_Encoder_GetDelta(void);    /* 上次的脉冲增量 */
int32_t BSP_Encoder_GetPosition(void); /* 累计脉冲（停电前不清零） */
void    BSP_Encoder_ResetPosition(void);

#endif /* __BSP_ENCODER_H */
