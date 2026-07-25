#ifndef UART_H
#define UART_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

/* 初始化 UART0 调试串口（SYSCFG_DL_init 之后调用） */
void UART_Init(void);

/* 发送单个字节 */
void UART_SendByte(uint8_t data);

/* 发送字符串（以 '\0' 结尾） */
void UART_SendStr(const char *str);

/* 格式化打印（支持 %d %u %x %s %c %f，自动追加 \r\n） */
void UART_Printf(const char *fmt, ...);

#endif
