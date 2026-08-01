/*
 * uart.h - Dual UART Driver for MSPM0G3507
 *
 * UART_K230 (UART1, PB7=RX, PB6=TX) — K230 ball position data input
 * UART_DEBUG (UART0, PA10=TX, PA11=RX) — PC serial monitor via XDS110
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>

/*==================== K230 UART (UART1) ====================*/

void uart_k230_init(void);
uint8_t uart_k230_available(void);     /* 返回可读字节数 (0 或 1) */
uint8_t uart_k230_read(uint8_t *byte); /* 非阻塞读，成功返回 1 */

/*==================== Debug UART (UART0 → PC) ====================*/

void uart_debug_init(void);
void uart_debug_putc(char c);
void uart_debug_puts(const char *s);
void uart_debug_putint(int32_t val);
void uart_debug_putfloat(float val, uint8_t decimals);
void uart_debug_printf(const char *fmt, ...);  /* 简易 printf */

/*==================== 统一初始化 ====================*/

void uart_init(void);  /* 调用上面两个 init + 打印启动信息 */

#endif
