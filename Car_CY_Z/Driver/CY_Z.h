#ifndef MODULE_CY_Z_H_
#define MODULE_CY_Z_H_

#include <stdbool.h>
#include <stdint.h>

/*
 * UART1 接 CY-Z：PA17(TX) -> 模块 RX，PA18(RX) <- 模块 TX。
 * UART0 的 PA10(TX) 用来打印调试信息。两个串口目前都是 115200。
 *
 * main() 里先跑 SYSCFG_DL_init()，再调 CY_Z_Init()。之后在循环里用
 * CY_Z_GetTelemetry() 取数据，返回 true 才说明这次拿到了新的一帧。
 *
 * 接线时别忘了 TX/RX 交叉、两边共地。下面的接口大多不检查空指针，
 * 也不要在中断里打印；这里的串口发送是阻塞的，打得太勤会拖慢主循环。
 */

#define CY_Z_TELEMETRY_FRAME_SIZE (16U)
#define CY_Z_COMMAND_FRAME_SIZE   (8U)
#define CY_Z_ACK_FRAME_SIZE       (8U)

typedef struct {
    uint16_t seq;
    float angle_deg;
    float gyro_dps;
} CY_Z_Telemetry;

typedef struct {
    uint8_t cmd;
    uint8_t result;
    uint8_t seq;
} CY_Z_Ack;

typedef enum {
    CY_Z_RESULT_OK = 0x00,
    CY_Z_RESULT_NOT_STATIC = 0x01,
    CY_Z_RESULT_BAD_COMMAND = 0x02,
    CY_Z_RESULT_FAILED = 0x03
} CY_Z_Result;

typedef enum {
    CY_Z_RATE_50HZ = 0x00,
    CY_Z_RATE_20HZ = 0x01,
    CY_Z_RATE_10HZ = 0x02,
    CY_Z_RATE_QUERY_ONLY = 0x03
} CY_Z_ReportRate;

/* 清空接收状态并打开 UART1 中断。必须放在 SYSCFG_DL_init() 后面。 */
void CY_Z_Init(void);

/* 取走最新遥测。成功后会清掉新数据标志，telemetry 不能传 NULL。 */
bool CY_Z_GetTelemetry(CY_Z_Telemetry *telemetry);

/* 取走最新 ACK。只保留最后一帧，读取成功后会清标志，ack 不能传 NULL。 */
bool CY_Z_GetAck(CY_Z_Ack *ack);

/* 从 UART0 打印字符串。阻塞发送，text 不能为 NULL，也要保证末尾有 '\0'。 */
void CY_Z_PrintText(const char *text);

/* 打印序号、角度和角速度。里面用了 %f，会多占一些 Flash。 */
void CY_Z_PrintTelemetry(const CY_Z_Telemetry *telemetry);

/* 打印 ACK。result 等于 0 才是模块执行成功。 */
void CY_Z_PrintAck(const CY_Z_Ack *ack);

/* 打印接收统计。crc_err 目前还没接上计数逻辑，暂时不要拿它判断 CRC。 */
void CY_Z_PrintStatus(void);

/* 把一条命令组好后按十六进制打印，排查 CRC 或帧格式时用。 */
void CY_Z_PrintCommandFrame(uint8_t cmd, uint8_t param, uint8_t seq);

/* 角度归零。模块要放稳；seq 每次加 1，255 后自然回到 0 即可。 */
void CY_Z_SendZeroAngle(uint8_t seq);

/* 测试用固定归零帧，seq 固定为 0x5B。改任一字节后原 CRC 就不对了。 */
void CY_Z_SendZeroAngleFixed(void);

/* 重新估计零偏，执行时模块要保持静止。 */
void CY_Z_SendRecalibrateBias(uint8_t seq);

/* 查询当前上报频率，结果从 CY_Z_GetAck() 取。 */
void CY_Z_RequestReportRate(uint8_t seq);

/* 设置上报频率。QUERY_ONLY 模式不会主动上报，要手动请求数据。 */
void CY_Z_SetReportRate(CY_Z_ReportRate rate, uint8_t seq);

/* 查询模式下手动请求一帧遥测。 */
void CY_Z_RequestTelemetry(uint8_t seq);

/* 只发送比例因子查询；12 字节的返回帧目前还没有解析。 */
void CY_Z_RequestScaleFactor(uint8_t seq);

/* 只发送进入 Bootloader 的命令，后面的升级分包流程还没有实现。 */
void CY_Z_EnterBootloader(uint8_t seq);

/* UART1 接收处理，由下面的中断入口调用，应用层不用手动调用。 */
void CY_Z_UART1_IRQHandler(void);

#endif /* MODULE_CY_Z_H_ */
