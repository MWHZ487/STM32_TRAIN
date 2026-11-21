/**
 * @file    uart.h
 * @brief   串口通信（DMA 发送 + DMA+IDLE 接收）—— 机试库
 * @note    依赖 CubeMX 生成的 huart1，波特率 115200 8N1
 *
 *  用法：
 *      UART_Init(&huart1);                  // 在 MX_USART1_UART_Init() 之后
 *      UART_Printf("%d,%d\n", a, b);        // DMA 非阻塞发送
 *      uint8_t buf[64];
 *      uint16_t n = UART_ReadFrame(buf, sizeof buf);   // 取一帧（非0表示有新数据）
 */
#ifndef __UART_H
#define __UART_H

#include "main.h"
#include <stdint.h>

#define UART_RX_MAX   128       /* 单帧最大字节数 */
#define UART_TX_MAX   256       /* 发送缓冲大小 */

void     UART_Init(UART_HandleTypeDef *huart);
void     UART_Send(const uint8_t *data, uint16_t len);
uint16_t UART_ReadFrame(uint8_t *dst, uint16_t max);
int      UART_Printf(const char *fmt, ...);

#endif /* __UART_H */
