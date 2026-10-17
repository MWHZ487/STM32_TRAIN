/**
 ******************************************************************************
 * @file    main.c
 * @brief   HAL 版 —— PC13 板载 LED 闪烁（使用 ST HAL 库）
 *
 * 【与裸机版的区别】
 *   01_blink_register  直接写寄存器，能看到每个 bit 的含义
 *   02_blink_hal       调用 HAL 库函数，代码更像"业务逻辑"
 *
 *   两种都要会：裸机版帮你理解底层，HAL 版是实际项目的写法。
 *
 * 【硬件】STM32F103C8T6 (Blue Pill)，LED 接 PC13，低电平点亮
 ******************************************************************************
 */

#include "main.h"

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);

int main(void)
{
    /* 第 1 步：HAL 库初始化
     * 这个函数内部会：
     *   - 把 SysTick 定时器配成 1ms 中断一次（HAL_Delay 的基础）
     *   - 复位所有外设
     *   - 初始化 Flash 预取缓冲
     */
    HAL_Init();

    /* 第 2 步：把系统时钟从默认的 8MHz 拉到 72MHz */
    SystemClock_Config();

    /* 第 3 步：配置我们用到的 GPIO */
    MX_GPIO_Init();

    /* 第 4 步：主循环 —— 每 500ms 翻转一次 LED */
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(500);
    }
}

/**
 * @brief 系统时钟配置
 *
 * 目标：SYSCLK = 72MHz（STM32F103 的最高频率）
 *
 * 路径：HSE(8MHz 外部晶振) → PLL ×9 → 72MHz → SYSCLK
 *
 * 注意：这条路径依赖板子上的 8MHz 晶振起振。
 *       如果晶振有问题（焊接不良、买到假晶振），会卡在 Error_Handler。
 *       排查方法见工程 README 或问我。
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* ---- 振荡器 + PLL 配置 ---- */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;   /* 8MHz × 9 = 72MHz */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* ---- 时钟树分频配置 ---- */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK   | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;   /* HCLK  = 72MHz */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;     /* PCLK1 = 36MHz（APB1 上限 36MHz）*/
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;     /* PCLK2 = 72MHz */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief GPIO 初始化
 *
 * 对比裸机版：那边要手动置 RCC_APB2ENR 的 bit4、手动清写 CRH 的 4 个 bit。
 * 这里 HAL 把它包装成了几个语义清晰的调用。
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 打开 GPIOC 时钟（等价于裸机版的手动置位） */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* 先把 PC13 置高：Blue Pill 的 LED 低电平点亮，置高 = 先熄灭 */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;      /* 推挽输出 */
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;      /* 点灯不需要高速 */
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/**
 * @brief 出错时的死循环
 *
 * 调试时可以在这里打个断点，就知道是哪一步配置失败了。
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
