/**
 * @file    bsp_encoder.c
 * @brief   编码器：读计数 → 算增量 → 算 RPM → 累加位置
 *
 *  为什么必须 (int16_t) 强转（面试会问）：
 *      TIM2 的 CNT 是 16 位，从 65535 溢出回 0。
 *      直接相减：5 - 65530 = -65525（错）
 *      强转成 int16_t 后按补码解释：(int16_t)5 - (int16_t)65530 = 5 - (-6) = 11（对）
 *      这样正反转、长距离累计都不会出错。
 *
 *  位置为什么不溢出：
 *      delta 是 int16_t（±32767），用 int32_t 累加可以撑 2^31 个脉冲，
 *      1320 脉冲/圈 → 约 160 万圈，够用了。
 */
#include "bsp_encoder.h"

extern TIM_HandleTypeDef htim2;

static int16_t s_last_cnt = 0;
static int16_t s_delta    = 0;
static int32_t s_position = 0;

void BSP_Encoder_Init(void)
{
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);   /* 必须 ALL，不是 CH1 */
    s_last_cnt = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
    s_delta    = 0;
    s_position = 0;
}

float BSP_Encoder_ReadRpm(void)
{
    int16_t now = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);

    s_delta = now - s_last_cnt;
    s_last_cnt = now;
    s_position += s_delta;

    /* RPM = (delta / PPR) / T × 60 = delta × 60 / (PPR × T) */
    return (float)s_delta * 60.0f / (ENC_PPR * ENC_PERIOD_S);
}

int16_t BSP_Encoder_GetDelta(void)    { return s_delta; }
int32_t BSP_Encoder_GetPosition(void) { return s_position; }
void    BSP_Encoder_ResetPosition(void){ s_position = 0; }
