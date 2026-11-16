/**
 * @file    app_wave.h
 * @brief   三种目标波形（阶跃 / 固定正弦 / 变频正弦）—— 机试库
 *
 *  用法（在 10ms 中断里）：
 *      g_t += 0.01f;
 *      int32_t target = APP_Wave_Get(g_wave_mode, g_t);
 *      float u = PID_Calc(&spd, (float)target, rpm);
 */
#ifndef __APP_WAVE_H
#define __APP_WAVE_H

#include <stdint.h>

typedef enum {
    WAVE_STEP = 0,        /* 阶跃 */
    WAVE_SINE_FIX,        /* 固定频率 + 固定幅度正弦 */
    WAVE_SINE_SWEEP,      /* 变频 + 变幅正弦 */
    WAVE_MODE_NUM
} WaveMode;

int32_t     APP_Wave_Get(WaveMode mode, float t);
const char *APP_Wave_Name(WaveMode mode);

#endif /* __APP_WAVE_H */
