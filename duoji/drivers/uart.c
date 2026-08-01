/*
 * uart.c - Dual UART Driver
 *
 * UART_K230 (UART1, PB7=RX, PB6=TX) — K230 ball position data input
 * UART_DEBUG (UART0, PA10=TX, PA11=RX) — PC serial monitor
 */

#include "ti_msp_dl_config.h"
#include "uart.h"
#include <stdarg.h>

/*============================================================================
 * K230 UART (UART1) — 非阻塞接收
 *============================================================================*/

void uart_k230_init(void)
{
    /* 已由 SYSCFG_DL_UART_K230_init() 完成，此处预留扩展 */
}

uint8_t uart_k230_available(void)
{
    /* DL_UART 没有直接查 FIFO 深度的 API，用 receiveDataCheck 试读 */
    uint8_t dummy;
    return DL_UART_Main_receiveDataCheck(UART_K230_INST, &dummy) ? 1 : 0;
}

uint8_t uart_k230_read(uint8_t *byte)
{
    return DL_UART_Main_receiveDataCheck(UART_K230_INST, byte) ? 1 : 0;
}

/*============================================================================
 * Debug UART (UART0 → PC) — 阻塞发送
 *============================================================================*/

void uart_debug_init(void)
{
    /* 已由 SYSCFG_DL_UART_DEBUG_init() 完成 */
}

void uart_debug_putc(char c)
{
    DL_UART_Main_transmitDataBlocking(UART_0_INST, (uint8_t)c);
}

void uart_debug_puts(const char *s)
{
    while (*s) {
        uart_debug_putc(*s++);
    }
}

void uart_debug_putint(int32_t val)
{
    char buf[12];
    uint8_t i = sizeof(buf);
    uint8_t neg = 0;

    if (val == 0) { uart_debug_putc('0'); return; }
    if (val < 0) { neg = 1; val = -val; }

    buf[--i] = '\0';
    while (val > 0) { buf[--i] = '0' + (val % 10); val /= 10; }
    if (neg) buf[--i] = '-';

    uart_debug_puts(&buf[i]);
}

void uart_debug_putfloat(float val, uint8_t decimals)
{
    int32_t ipart, fpart;
    uint8_t d;
    float factor = 1.0f;
    char fb[8];
    uint8_t fi;

    /* 处理负零 */
    if (val < 0 && val > -1.0f) uart_debug_putc('-');

    for (d = 0; d < decimals; d++) factor *= 10.0f;

    if (val < 0) {
        uart_debug_putc('-');
        ipart = -(int32_t)val;
        fpart = (int32_t)((-val - (float)ipart) * factor + 0.5f);
    } else {
        ipart = (int32_t)val;
        fpart = (int32_t)((val - (float)ipart) * factor + 0.5f);
    }
    if (fpart >= (int32_t)factor) { ipart++; fpart -= (int32_t)factor; }

    uart_debug_putint(ipart);
    if (decimals > 0) {
        uart_debug_putc('.');
        fi = sizeof(fb);
        fb[--fi] = '\0';
        for (d = 0; d < decimals; d++) {
            fb[--fi] = '0' + (fpart % 10);
            fpart /= 10;
        }
        uart_debug_puts(&fb[fi]);
    }
}

/* 简易 printf：支持 %d, %f, %s, %c, %% */
void uart_debug_printf(const char *fmt, ...)
{
    va_list args;
    const char *p;
    int32_t ival;
    float fval;
    char *sval;
    uint8_t decimals;

    va_start(args, fmt);

    for (p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            uart_debug_putc(*p);
            continue;
        }

        p++; /* 跳过 '%' */

        /* 解析小数位数 %.Nf */
        decimals = 2; /* 默认 2 位 */
        if (*p == '.') {
            p++;
            decimals = 0;
            while (*p >= '0' && *p <= '9') {
                decimals = decimals * 10 + (*p - '0');
                p++;
            }
        }

        switch (*p) {
        case 'd':
            ival = va_arg(args, int32_t);
            uart_debug_putint(ival);
            break;
        case 'f':
            fval = (float)va_arg(args, double); /* C 默认提升 float→double */
            uart_debug_putfloat(fval, decimals);
            break;
        case 's':
            sval = va_arg(args, char *);
            uart_debug_puts(sval ? sval : "(null)");
            break;
        case 'c':
            uart_debug_putc((char)va_arg(args, int));
            break;
        case '%':
            uart_debug_putc('%');
            break;
        default:
            uart_debug_putc('%');
            uart_debug_putc(*p);
            break;
        }
    }

    va_end(args);
}

/*============================================================================
 * 统一初始化
 *============================================================================*/

void uart_init(void)
{
    uart_debug_puts("\r\n================================\r\n");
    uart_debug_puts("  K230 UART Bridge\r\n");
    uart_debug_puts("  K230 -> PB6/PB7 (UART1)\r\n");
    uart_debug_puts("  PC   <- PA10/PA11 (UART0)\r\n");
    uart_debug_puts("================================\r\n\r\n");
}
