/**
 ******************************************************************************
 * @file    stm32f1xx_it.c
 * @brief   中断服务程序（Interrupt Service Routines）
 *
 * 【为什么必须有这个文件】
 *   最关键的是最下面的 SysTick_Handler。
 *   HAL_Delay() 的实现是"记录起始 tick，然后死等直到差值够"，
 *   而 tick 这个计数器由 SysTick 中断每秒加 1000 次。
 *   少了这个函数，HAL_Delay 会永久卡死 —— 灯不闪，程序也不报错，很难查。
 *
 *   启动文件里把这些中断都声明成了弱符号（指向死循环），
 *   这里给出同名函数就能覆盖掉它们。
 ******************************************************************************
 */

#include "main.h"

/* ---------------------------------------------------------------------------
 * Cortex-M3 内核异常
 * --------------------------------------------------------------------------- */

void NMI_Handler(void)        { while (1) { } }
void HardFault_Handler(void)  { while (1) { } }
void MemManage_Handler(void)  { while (1) { } }
void BusFault_Handler(void)   { while (1) { } }
void UsageFault_Handler(void) { while (1) { } }
void SVC_Handler(void)        { }
void DebugMon_Handler(void)   { }
void PendSV_Handler(void)     { }

/* ---------------------------------------------------------------------------
 * SysTick 中断：HAL 的 1ms 心跳
 * --------------------------------------------------------------------------- */
void SysTick_Handler(void)
{
    HAL_IncTick();
}
