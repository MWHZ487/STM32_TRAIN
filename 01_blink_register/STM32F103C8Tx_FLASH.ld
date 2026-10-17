/*
 * STM32F103C8T6 链接脚本
 *
 * 链接脚本的作用：告诉链接器"把代码放哪、把变量放哪"
 * 单片机没有操作系统，内存布局必须写死。
 *
 * 本芯片资源：
 *   Flash 64KB  @ 0x08000000   （掉电不丢，放程序和常量）
 *   RAM   20KB  @ 0x20000000   （掉电即丢，放变量）
 */

ENTRY(Reset_Handler)

/* 栈顶设在 RAM 最高地址，栈从高地址往低地址生长 */
_estack = ORIGIN(RAM) + LENGTH(RAM);

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
  /* ------------------------------------------------------------------
   * 中断向量表必须放在 Flash 最开头
   * CPU 复位后固定从 0x08000000 读取栈顶地址和复位入口，
   * 位置一错就永远进不了 main
   * ------------------------------------------------------------------ */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* ------------------------------------------------------------------
   * 代码段：所有函数编译成的机器码
   * ------------------------------------------------------------------ */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  /* ------------------------------------------------------------------
   * 只读数据：const 变量、字符串字面量
   * ------------------------------------------------------------------ */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH

  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* ------------------------------------------------------------------
   * .data 段：有初值的全局/静态变量
   *
   * 关键点：初值存在 Flash（省不下），但运行时必须在 RAM
   * 所以链接器同时给出两份地址：
   *   >RAM AT> FLASH  表示"运行时地址在 RAM，装载地址在 Flash"
   * 启动文件里的 CopyDataInit 循环负责把它们搬过去
   * ------------------------------------------------------------------ */
  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  /* ------------------------------------------------------------------
   * .bss 段：没有初值的全局/静态变量，C 标准要求必须为 0
   * 启动文件里的 FillZerobss 循环负责清零
   * ------------------------------------------------------------------ */
  . = ALIGN(4);
  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /* ------------------------------------------------------------------
   * 给堆和栈预留空间，避免链接器把它们排在 .bss 后面
   * ------------------------------------------------------------------ */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + 0x400;    /* 栈空间 1KB */
    . = . + 0x200;    /* 堆空间 512B */
    . = ALIGN(8);
  } >RAM

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
