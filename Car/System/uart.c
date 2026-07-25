#include "uart.h"
#include <stdio.h>
#include <stdarg.h>

/*==================================================================
 *  UART_Init — 初始化 UART0 调试串口
 *  SYSCFG_DL_init() 已配置 UART_0 (PA10 TX, 115200, TX-only)，
 *  这里显式使能以确保可用
 *==================================================================*/
void UART_Init(void)
{
    DL_UART_Main_enable(UART_0_INST);
}

/*==================================================================
 *  UART_SendByte — 阻塞发送单字节
 *==================================================================*/
void UART_SendByte(uint8_t data)
{
    DL_UART_Main_transmitDataBlocking(UART_0_INST, data);
}

/*==================================================================
 *  UART_SendStr — 阻塞发送字符串
 *==================================================================*/
void UART_SendStr(const char *str)
{
    while (*str != '\0') {
        UART_SendByte((uint8_t)*str);
        str++;
    }
}

/*==================================================================
 *  UART_Printf — 格式化打印，自动追加 \r\n
 *==================================================================*/
void UART_Printf(const char *fmt, ...)
{
    char buf[128];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    UART_SendStr(buf);
    UART_SendStr("\r\n");
}
