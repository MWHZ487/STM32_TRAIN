/**
 * @file    key.h
 * @brief   2~3 个按键中断读取（EXTI + 时间窗消抖）—— 机试库
 *
 *  CubeMX 配置：
 *      引脚 → GPIO_EXTIx，下降沿触发，上拉（Pull-up）
 *      NVIC → 使能对应的 EXTI line 中断
 *
 *  用法：
 *      KEY_Init();
 *      if (KEY_GetEvent(0)) { ... }     // 0/1/2 对应 KEY1/2/3，读后自动清除
 */
#ifndef __KEY_H
#define __KEY_H

#include "main.h"
#include <stdint.h>

#define KEY_NUM      3

/* 按实际接线修改端口与引脚 */
#define KEY1_PORT    GPIOB
#define KEY1_PIN     GPIO_PIN_0

#define KEY2_PORT    GPIOB
#define KEY2_PIN     GPIO_PIN_1

#define KEY3_PORT    GPIOB
#define KEY3_PIN     GPIO_PIN_5

#define KEY_DEBOUNCE_MS   50      /* 时间窗消抖：50ms 内只认一次 */

void    KEY_Init(void);
uint8_t KEY_GetEvent(uint8_t idx);   /* idx: 0~2，有新按键返回 1 并自动清除 */
uint8_t KEY_Raw(uint8_t idx);        /* 直接读电平，0 = 按下（上拉） */

#endif /* __KEY_H */
