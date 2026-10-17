/**
 ******************************************************************************
 * @file      startup_stm32f103xb.s
 * @brief     STM32F103xB (Blue Pill) 启动文件 —— GNU as / arm-none-eabi 语法
 *
 * 复位后 CPU 做的三件事：
 *   1. 从向量表第 0 项取出栈顶地址，装进 SP
 *   2. 从向量表第 1 项取出 Reset_Handler 地址，跳过去执行
 *   3. Reset_Handler 负责把 C 语言的运行环境准备好，最后跳进 main
 ******************************************************************************
 */

  .syntax unified
  .cpu cortex-m3
  .fpu softvfp
  .thumb

.global  g_pfnVectors
.global  Default_Handler

/* 这些符号由链接脚本 STM32F103C8Tx_FLASH.ld 提供 */
.word  _sidata
.word  _sdata
.word  _edata
.word  _sbss
.word  _ebss

/**
 * 复位处理程序
 */
  .section  .text.Reset_Handler
  .weak  Reset_Handler
  .type  Reset_Handler, %function
Reset_Handler:
  /* ---- 第 1 步：把已初始化变量的初值从 Flash 搬到 RAM ----
   * .data 段里存放的是 int a = 5; 这类变量
   * 它们的初值躺在 Flash，上电后必须复制到 RAM 才能真正当变量用
   */
  ldr   r0, =_sdata
  ldr   r1, =_edata
  ldr   r2, =_sidata
  movs  r3, #0
  b     LoopCopyDataInit

CopyDataInit:
  ldr   r4, [r2, r3]
  str   r4, [r0, r3]
  adds  r3, r3, #4

LoopCopyDataInit:
  adds  r4, r0, r3
  cmp   r4, r1
  bcc   CopyDataInit

  /* ---- 第 2 步：把未初始化变量区清零 ----
   * .bss 段里是 int flag; 这类没有初值的变量
   * C 标准要求它们必须是 0，所以逐字节清零
   */
  ldr   r2, =_sbss
  ldr   r4, =_ebss
  movs  r3, #0
  b     LoopFillZerobss

FillZerobss:
  str   r3, [r2], #4

LoopFillZerobss:
  cmp   r2, r4
  bcc   FillZerobss

  /* ---- 第 3 步：进入 C 世界 ---- */
  bl    main
  bx    lr

.size  Reset_Handler, .-Reset_Handler

/**
 * 未实现的中断统一停在这里，方便调试时定位
 */
  .section  .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b  Infinite_Loop
  .size  Default_Handler, .-Default_Handler

/**
 * 中断向量表
 * 位置强制放在 Flash 最开头，因为 CPU 固定从 0x08000000 取这张表
 */
  .section  .isr_vector,"a",%progbits
  .type  g_pfnVectors, %object

g_pfnVectors:
  .word  _estack                 /* 栈顶地址 */
  .word  Reset_Handler           /* 复位入口 */
  .word  NMI_Handler
  .word  HardFault_Handler
  .word  MemManage_Handler
  .word  BusFault_Handler
  .word  UsageFault_Handler
  .word  0
  .word  0
  .word  0
  .word  0
  .word  SVC_Handler
  .word  DebugMon_Handler
  .word  0
  .word  PendSV_Handler
  .word  SysTick_Handler

  /* 以下为 STM32F103xB 的外部中断，共 43 个 */
  .word  WWDG_IRQHandler
  .word  PVD_IRQHandler
  .word  TAMPER_IRQHandler
  .word  RTC_IRQHandler
  .word  FLASH_IRQHandler
  .word  RCC_IRQHandler
  .word  EXTI0_IRQHandler
  .word  EXTI1_IRQHandler
  .word  EXTI2_IRQHandler
  .word  EXTI3_IRQHandler
  .word  EXTI4_IRQHandler
  .word  DMA1_Channel1_IRQHandler
  .word  DMA1_Channel2_IRQHandler
  .word  DMA1_Channel3_IRQHandler
  .word  DMA1_Channel4_IRQHandler
  .word  DMA1_Channel5_IRQHandler
  .word  DMA1_Channel6_IRQHandler
  .word  DMA1_Channel7_IRQHandler
  .word  ADC1_2_IRQHandler
  .word  USB_HP_CAN1_TX_IRQHandler
  .word  USB_LP_CAN1_RX0_IRQHandler
  .word  CAN1_RX1_IRQHandler
  .word  CAN1_SCE_IRQHandler
  .word  EXTI9_5_IRQHandler
  .word  TIM1_BRK_IRQHandler
  .word  TIM1_UP_IRQHandler
  .word  TIM1_TRG_COM_IRQHandler
  .word  TIM1_CC_IRQHandler
  .word  TIM2_IRQHandler
  .word  TIM3_IRQHandler
  .word  TIM4_IRQHandler
  .word  I2C1_EV_IRQHandler
  .word  I2C1_ER_IRQHandler
  .word  I2C2_EV_IRQHandler
  .word  I2C2_ER_IRQHandler
  .word  SPI1_IRQHandler
  .word  SPI2_IRQHandler
  .word  USART1_IRQHandler
  .word  USART2_IRQHandler
  .word  USART3_IRQHandler
  .word  EXTI15_10_IRQHandler
  .word  RTCAlarm_IRQHandler
  .word  USBWakeUp_IRQHandler
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0

  .size  g_pfnVectors, .-g_pfnVectors

/**
 * 用宏批量把中断声明为弱符号，默认指向 Default_Handler
 * 你以后写同名函数就能自动覆盖它
 */
  .macro  IRQ handler
  .weak  \handler
  .thumb_set \handler,Default_Handler
  .endm

  IRQ  NMI_Handler
  IRQ  HardFault_Handler
  IRQ  MemManage_Handler
  IRQ  BusFault_Handler
  IRQ  UsageFault_Handler
  IRQ  SVC_Handler
  IRQ  DebugMon_Handler
  IRQ  PendSV_Handler
  IRQ  SysTick_Handler

  IRQ  WWDG_IRQHandler
  IRQ  PVD_IRQHandler
  IRQ  TAMPER_IRQHandler
  IRQ  RTC_IRQHandler
  IRQ  FLASH_IRQHandler
  IRQ  RCC_IRQHandler
  IRQ  EXTI0_IRQHandler
  IRQ  EXTI1_IRQHandler
  IRQ  EXTI2_IRQHandler
  IRQ  EXTI3_IRQHandler
  IRQ  EXTI4_IRQHandler
  IRQ  DMA1_Channel1_IRQHandler
  IRQ  DMA1_Channel2_IRQHandler
  IRQ  DMA1_Channel3_IRQHandler
  IRQ  DMA1_Channel4_IRQHandler
  IRQ  DMA1_Channel5_IRQHandler
  IRQ  DMA1_Channel6_IRQHandler
  IRQ  DMA1_Channel7_IRQHandler
  IRQ  ADC1_2_IRQHandler
  IRQ  USB_HP_CAN1_TX_IRQHandler
  IRQ  USB_LP_CAN1_RX0_IRQHandler
  IRQ  CAN1_RX1_IRQHandler
  IRQ  CAN1_SCE_IRQHandler
  IRQ  EXTI9_5_IRQHandler
  IRQ  TIM1_BRK_IRQHandler
  IRQ  TIM1_UP_IRQHandler
  IRQ  TIM1_TRG_COM_IRQHandler
  IRQ  TIM1_CC_IRQHandler
  IRQ  TIM2_IRQHandler
  IRQ  TIM3_IRQHandler
  IRQ  TIM4_IRQHandler
  IRQ  I2C1_EV_IRQHandler
  IRQ  I2C1_ER_IRQHandler
  IRQ  I2C2_EV_IRQHandler
  IRQ  I2C2_ER_IRQHandler
  IRQ  SPI1_IRQHandler
  IRQ  SPI2_IRQHandler
  IRQ  USART1_IRQHandler
  IRQ  USART2_IRQHandler
  IRQ  USART3_IRQHandler
  IRQ  EXTI15_10_IRQHandler
  IRQ  RTCAlarm_IRQHandler
  IRQ  USBWakeUp_IRQHandler

  .end
