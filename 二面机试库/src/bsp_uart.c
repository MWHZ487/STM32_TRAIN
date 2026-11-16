/**
 * @file    bsp_uart.c
 * @brief   串口：DMA 发送 + DMA/IDLE 接收 + printf 重定向
 *
 *  接收原理：
 *      HAL_UARTEx_ReceiveToIdle_DMA() 让 DMA 一直收，收到数据就搬进缓冲。
 *      对方停止发送（总线空闲 IDLE）时 HAL 调 HAL_UARTEx_RxEventCallback，
 *      告诉我们这一帧多长 —— 这样不定长帧也能正确分界。
 *
 *  注意：如果工程里另外定义了 _write（printf 重定向），把这个文件里的
 *        _write 删掉，只保留一处。
 */
#include "bsp_uart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef *s_huart   = NULL;
static uint8_t             s_rx_buf[UART_RX_MAX];
static uint8_t             s_tx_buf[UART_TX_MAX];
static volatile uint16_t   s_rx_len  = 0;
static volatile uint8_t    s_rx_flag = 0;

void BSP_UART_Init(UART_HandleTypeDef *huart)
{
    s_huart   = huart;
    s_rx_len  = 0;
    s_rx_flag = 0;

    /* DMA 循环接收，靠 IDLE 事件分帧 */
    HAL_UARTEx_ReceiveToIdle_DMA(huart, s_rx_buf, UART_RX_MAX);
}

void BSP_UART_Send(const uint8_t *data, uint16_t len)
{
    if (s_huart == NULL || data == NULL || len == 0)
        return;

    if (len > UART_TX_MAX)
        len = UART_TX_MAX;

    /* 先等上一帧发完，否则会覆盖正在被 DMA 搬运的缓冲 */
    uint32_t t0 = HAL_GetTick();
    while (s_huart->gState != HAL_UART_STATE_READY)
    {
        if (HAL_GetTick() - t0 > 50)   /* 超时就不发，避免卡死 */
            return;
    }

    memcpy(s_tx_buf, data, len);
    HAL_UART_Transmit_DMA(s_huart, s_tx_buf, len);
}

uint16_t BSP_UART_ReadFrame(uint8_t *dst, uint16_t max)
{
    uint16_t n;

    if (!s_rx_flag || dst == NULL)
        return 0;

    s_rx_flag = 0;
    n = s_rx_len;
    if (n > max)
        n = max;
    memcpy(dst, s_rx_buf, n);
    return n;
}

int BSP_UART_Printf(const char *fmt, ...)
{
    char    buf[128];
    va_list ap;
    int     n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);

    if (n > 0)
        BSP_UART_Send((const uint8_t *)buf, (uint16_t)n);
    return n;
}

/* ==================== HAL 回调：一帧接收完成 ==================== */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == s_huart)
    {
        s_rx_len  = Size;
        s_rx_flag = 1;
        /* 必须重新挂上，否则只能收一帧 */
        HAL_UARTEx_ReceiveToIdle_DMA(huart, s_rx_buf, UART_RX_MAX);
    }
}

/* ==================== printf 重定向 ==================== */
int _write(int fd, char *ptr, int len)
{
    (void)fd;
    BSP_UART_Send((const uint8_t *)ptr, (uint16_t)len);
    return len;
}
