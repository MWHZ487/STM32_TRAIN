/**
 * @file    bsp_key.c
 * @brief   按键中断：EXTI 回调里只置标志，时间窗消抖由"记录上次接受时刻"实现
 *
 *  为什么这样消抖（面试可讲）：
 *      机械按键按下/松开会在几毫秒内产生多次抖动。
 *      EXTI 是"边沿触发"，本来就只认一次下降沿，但抖动仍可能产生多个边沿。
 *      所以回调里不直接处理业务，只做一件事：
 *          如果距离上次被接受的时间 > 50ms，才算一次有效按下
 *  优点：中断里执行时间极短（只有一次加法和比较），不阻塞其他中断。
 */
#include "bsp_key.h"

static GPIO_TypeDef *const s_port[KEY_NUM] = { KEY1_PORT, KEY2_PORT, KEY3_PORT };
static const uint16_t      s_pin [KEY_NUM] = { KEY1_PIN,  KEY2_PIN,  KEY3_PIN  };
static uint32_t            s_last_tick[KEY_NUM];
static volatile uint8_t    s_event   [KEY_NUM];

void BSP_Key_Init(void)
{
    for (uint8_t i = 0; i < KEY_NUM; i++)
    {
        s_event[i]     = 0;
        s_last_tick[i] = 0;
    }
}

uint8_t BSP_Key_GetEvent(uint8_t idx)
{
    if (idx >= KEY_NUM)
        return 0;
    if (s_event[idx])
    {
        s_event[idx] = 0;
        return 1;
    }
    return 0;
}

uint8_t BSP_Key_Raw(uint8_t idx)
{
    if (idx >= KEY_NUM)
        return 1;
    return (HAL_GPIO_ReadPin(s_port[idx], s_pin[idx]) == GPIO_PIN_RESET) ? 0 : 1;
}

/* ==================== EXTI 回调（所有 EXTI 引脚共用） ==================== */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0; i < KEY_NUM; i++)
    {
        if (GPIO_Pin != s_pin[i])
            continue;

        if (s_last_tick[i] == 0 || (now - s_last_tick[i]) > KEY_DEBOUNCE_MS)
        {
            s_last_tick[i] = now;
            s_event[i]     = 1;
        }
        return;
    }
}
