#include "CY_Z.h"

#include "ti_msp_dl_config.h"

#include <stdio.h>
#include <string.h>

#define CY_Z_TELEMETRY_HEADER0 (0xAAU)
#define CY_Z_TELEMETRY_HEADER1 (0x55U)
#define CY_Z_TELEMETRY_TAIL0   (0x55U)
#define CY_Z_TELEMETRY_TAIL1   (0xAAU)

#define CY_Z_COMMAND_HEADER0 (0xA5U)
#define CY_Z_COMMAND_HEADER1 (0x5AU)
#define CY_Z_COMMAND_TAIL    (0x5AU)
#define CY_Z_ACK_HEADER0     (0xA5U)
#define CY_Z_ACK_HEADER1     (0x5BU)
#define CY_Z_ACK_TAIL        (0x5BU)

#define CY_Z_CMD_RESET          (0x01U)
#define CY_Z_CMD_GET_RATE       (0x02U)
#define CY_Z_CMD_SET_RATE       (0x03U)
#define CY_Z_CMD_QUERY_TELEMETRY (0x04U)
#define CY_Z_CMD_GET_SCALE      (0x08U)
#define CY_Z_CMD_BOOTLOADER     (0x09U)

static volatile uint8_t g_rx_window[CY_Z_TELEMETRY_FRAME_SIZE];
static volatile uint8_t g_rx_window_count;
static volatile bool g_frame_ready;
static volatile bool g_ack_ready;
uint32_t g_rx_byte_count;
uint32_t g_valid_frame_count;
uint32_t g_ack_frame_count;
/* 当前解析失败只返回 false，尚未区分 CRC、帧头和帧尾错误，
 * 因此这个计数目前不会自动增加，不能把 crc_err=0 当成“没有 CRC 错误”。 */
uint32_t g_crc_error_count;
uint32_t g_drop_byte_count;

/* 这些结构体由 UART1 中断写、由主循环读。
 * 读取时必须短暂关中断，防止复制到一半时被新帧覆盖。 */
static CY_Z_Telemetry g_last_telemetry;
static CY_Z_Ack g_last_ack;

/* Modbus CRC16，组帧和验帧都会用到。 */
static uint16_t CY_Z_Crc16Modbus(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFFU;

    for (uint16_t i = 0U; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0U; bit < 8U; bit++) {
            if ((crc & 0x0001U) != 0U) {
                crc = (crc >> 1U) ^ 0xA001U;
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

/* 从字节流里读一个 16 位小端数。 */
static uint16_t CY_Z_ReadLe16(const uint8_t *data)
{
    return (uint16_t) data[0] | ((uint16_t) data[1] << 8U);
}

/* 把协议里的 4 个小端字节还原成 float。 */
static float CY_Z_ReadLeFloat(const uint8_t *data)
{
    /* 协议按小端顺序传输 float 的 4 个原始字节。
     * memcpy 可避免通过指针强制转换造成的对齐和严格别名问题。
     * 前提是编译器的 float 为常见的 32 位 IEEE-754 格式。 */
    uint32_t raw = ((uint32_t) data[0]) |
        ((uint32_t) data[1] << 8U) |
        ((uint32_t) data[2] << 16U) |
        ((uint32_t) data[3] << 24U);
    float value;

    memcpy(&value, &raw, sizeof(value));
    return value;
}

/* 检查并拆出一帧 16 字节遥测数据。 */
static bool CY_Z_ParseTelemetryFrame(
    const uint8_t *frame, CY_Z_Telemetry *telemetry)
{
    uint16_t crc_received;
    uint16_t crc_calculated;

    if ((frame[0] != CY_Z_TELEMETRY_HEADER0) ||
        (frame[1] != CY_Z_TELEMETRY_HEADER1) ||
        (frame[14] != CY_Z_TELEMETRY_TAIL0) ||
        (frame[15] != CY_Z_TELEMETRY_TAIL1)) {
        return false;
    }

    crc_received = CY_Z_ReadLe16(&frame[12]);
    /* CRC 只覆盖序号、角度和角速度（frame[2]~frame[11]），
     * 不包含帧头、CRC 自身和帧尾。范围多一个或少一个字节都会校验失败。 */
    crc_calculated = CY_Z_Crc16Modbus(&frame[2], 10U);
    if (crc_received != crc_calculated) {
        return false;
    }

    telemetry->seq = CY_Z_ReadLe16(&frame[2]);
    telemetry->angle_deg = CY_Z_ReadLeFloat(&frame[4]);
    telemetry->gyro_dps = CY_Z_ReadLeFloat(&frame[8]);

    return true;
}

/* 检查并拆出一帧 8 字节 ACK。 */
static bool CY_Z_ParseAckFrame(const uint8_t *frame, CY_Z_Ack *ack)
{
    uint16_t crc_received;
    uint16_t crc_calculated;

    if ((frame[0] != CY_Z_ACK_HEADER0) ||
        (frame[1] != CY_Z_ACK_HEADER1) ||
        (frame[7] != CY_Z_ACK_TAIL)) {
        return false;
    }

    crc_received = CY_Z_ReadLe16(&frame[5]);
    crc_calculated = CY_Z_Crc16Modbus(&frame[2], 3U);
    if (crc_received != crc_calculated) {
        return false;
    }

    ack->cmd = frame[2];
    ack->result = frame[3];
    ack->seq = frame[4];

    return true;
}

/* UART0 发一个字符，给调试打印用。 */
static void CY_Z_Uart0WriteByte(uint8_t data)
{
    DL_UART_Main_transmitDataBlocking(UART_0_INST, data);
}

/* UART0 打印字符串，text 必须以 '\0' 结尾。 */
static void CY_Z_Uart0WriteString(const char *text)
{
    /* 本函数没有 NULL 检查，调用者必须保证 text 有效且以 '\0' 结尾。 */
    while (*text != '\0') {
        CY_Z_Uart0WriteByte((uint8_t) *text);
        text++;
    }
}

/* UART1 发一整段协议数据，发送期间会一直等。 */
static void CY_Z_Uart1WriteBuffer(const uint8_t *data, uint16_t length)
{
    /* transmitDataBlocking 会等待每个字节发送，数据越长占用 CPU 越久。
     * 不要在 UART 接收中断里调用本函数，避免中断执行时间过长。 */
    for (uint16_t i = 0U; i < length; i++) {
        DL_UART_Main_transmitDataBlocking(UART_1_INST, data[i]);
    }
}

/* 组一帧控制命令，frame 至少要有 8 字节。 */
static void CY_Z_BuildCommandFrame(
    uint8_t cmd, uint8_t param, uint8_t seq, uint8_t *frame)
{
    uint16_t crc;

    frame[0] = CY_Z_COMMAND_HEADER0;
    frame[1] = CY_Z_COMMAND_HEADER1;
    frame[2] = cmd;
    frame[3] = param;
    frame[4] = seq;
    crc = CY_Z_Crc16Modbus(&frame[2], 3U);
    frame[5] = (uint8_t) (crc & 0x00FFU);
    frame[6] = (uint8_t) (crc >> 8U);
    frame[7] = CY_Z_COMMAND_TAIL;
}

/* 组好命令后从 UART1 发给模块。 */
static void CY_Z_SendCommand(uint8_t cmd, uint8_t param, uint8_t seq)
{
    uint8_t frame[CY_Z_COMMAND_FRAME_SIZE];

    CY_Z_BuildCommandFrame(cmd, param, seq, frame);
    CY_Z_Uart1WriteBuffer(frame, CY_Z_COMMAND_FRAME_SIZE);
}

/* 把 ACK 结果码换成方便打印的短字符串。 */
static const char *CY_Z_ResultToString(uint8_t result)
{
    switch (result) {
        case CY_Z_RESULT_OK:
            return "ok";
        case CY_Z_RESULT_NOT_STATIC:
            return "not_static";
        case CY_Z_RESULT_BAD_COMMAND:
            return "bad_cmd";
        case CY_Z_RESULT_FAILED:
            return "failed";
        default:
            return "unknown";
    }
}

/* 清空接收状态并打开 UART1 中断，SysConfig 初始化后调用。 */
void CY_Z_Init(void)
{
    memset((void *) g_rx_window, 0, sizeof(g_rx_window));
    g_rx_window_count = 0U;
    g_frame_ready = false;
    g_ack_ready = false;
    g_rx_byte_count = 0U;
    g_valid_frame_count = 0U;
    g_ack_frame_count = 0U;
    g_crc_error_count = 0U;
    g_drop_byte_count = 0U;
    memset(&g_last_telemetry, 0, sizeof(g_last_telemetry));
    memset(&g_last_ack, 0, sizeof(g_last_ack));

    NVIC_ClearPendingIRQ(UART_1_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_1_INST_INT_IRQN);
}

/* 从中断侧取走最新遥测，拿到新数据时返回 true。 */
bool CY_Z_GetTelemetry(CY_Z_Telemetry *telemetry)
{
    bool has_frame = false;

    /* 这里会无条件重新开中断，所以只能在普通主循环中调用；
     * 不要在 ISR 或已经手动关中断的临界区中调用。telemetry 也不能为 NULL。 */
    __disable_irq();
    if (g_frame_ready) {
        *telemetry = g_last_telemetry;
        g_frame_ready = false;
        has_frame = true;
    }
    __enable_irq();

    return has_frame;
}

/* 从中断侧取走最新 ACK，发命令后在主循环里查。 */
bool CY_Z_GetAck(CY_Z_Ack *ack)
{
    bool has_ack = false;

    /* 与 GetTelemetry 相同：只能在普通主循环中调用，ack 不能为 NULL。
     * 读取成功后 g_ack_ready 会被清除，同一个 ACK 不能重复读取。 */
    __disable_irq();
    if (g_ack_ready) {
        *ack = g_last_ack;
        g_ack_ready = false;
        has_ack = true;
    }
    __enable_irq();

    return has_ack;
}

/* 从 UART0 打印一段文本。 */
void CY_Z_PrintText(const char *text)
{
    CY_Z_Uart0WriteString(text);
}

/* 把一帧遥测按可读格式打到 UART0。 */
void CY_Z_PrintTelemetry(const CY_Z_Telemetry *telemetry)
{
    char text[80];
    int length;

    /* %f 浮点格式化比较占 Flash；若以后发现链接失败或不显示小数，
     * 需要检查当前工具链是否启用了 printf 浮点支持。 */
    length = snprintf(text, sizeof(text), "CY_Z seq=%u angle=%.3fdeg gyro=%.3fdps\r\n",
        telemetry->seq, telemetry->angle_deg, telemetry->gyro_dps);
    if (length > 0) {
        CY_Z_Uart0WriteString(text);
    }
}

/* 把 ACK 内容打到 UART0。 */
void CY_Z_PrintAck(const CY_Z_Ack *ack)
{
    char text[80];
    int length;

    length = snprintf(text, sizeof(text),
        "CY_Z ack: cmd=0x%02X result=0x%02X(%s) seq=%u\r\n",
        ack->cmd, ack->result, CY_Z_ResultToString(ack->result), ack->seq);
    if (length > 0) {
        CY_Z_Uart0WriteString(text);
    }
}

/* 只打印组好的命令帧，排查协议时用，不会真的发给模块。 */
void CY_Z_PrintCommandFrame(uint8_t cmd, uint8_t param, uint8_t seq)
{
    uint8_t frame[CY_Z_COMMAND_FRAME_SIZE];
    char text[80];
    int length;

    CY_Z_BuildCommandFrame(cmd, param, seq, frame);
    length = snprintf(text, sizeof(text),
        "CY_Z cmd: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
        frame[0], frame[1], frame[2], frame[3],
        frame[4], frame[5], frame[6], frame[7]);
    if (length > 0) {
        CY_Z_Uart0WriteString(text);
    }
}

/* 打印几个接收计数，通信不通时先看这里。 */
void CY_Z_PrintStatus(void)
{
    char text[96];
    int length;
    uint32_t rx_bytes;
    uint32_t valid_frames;
    uint32_t ack_frames;
    uint32_t crc_errors;
    uint32_t dropped_bytes;

    __disable_irq();
    rx_bytes = g_rx_byte_count;
    valid_frames = g_valid_frame_count;
    ack_frames = g_ack_frame_count;
    crc_errors = g_crc_error_count;
    dropped_bytes = g_drop_byte_count;
    __enable_irq();

    length = snprintf(text, sizeof(text),
        "CY_Z status: rx=%u frame=%u ack=%u crc_err=%u drop=%u\r\n",
        (unsigned int) rx_bytes, (unsigned int) valid_frames,
        (unsigned int) ack_frames, (unsigned int) crc_errors,
        (unsigned int) dropped_bytes);
    if (length > 0) {
        CY_Z_Uart0WriteString(text);
    }
}

/* 角度归零，调用时模块要放稳。 */
void CY_Z_SendZeroAngle(uint8_t seq)
{
    CY_Z_SendCommand(CY_Z_CMD_RESET, 0x01U, seq);
}

/* 发固定的归零测试帧，平时优先用上面的可变 seq 版本。 */
void CY_Z_SendZeroAngleFixed(void)
{
    /* 这是整帧常量：第 5 个字节 0x5B 是固定 seq，后面的 CRC 也按它计算。
     * 若要换 seq，必须重新组帧，优先调用 CY_Z_SendZeroAngle(seq)。 */
    static const uint8_t frame[CY_Z_COMMAND_FRAME_SIZE] = {
        0xA5U, 0x5AU, 0x01U, 0x01U, 0x5BU, 0x60U, 0x6BU, 0x5AU
    };

    CY_Z_Uart1WriteBuffer(frame, CY_Z_COMMAND_FRAME_SIZE);
}

/* 重新估计零偏，执行时模块要保持静止。 */
void CY_Z_SendRecalibrateBias(uint8_t seq)
{
    CY_Z_SendCommand(CY_Z_CMD_RESET, 0x02U, seq);
}

/* 查询当前上报频率，结果从 ACK 里取。 */
void CY_Z_RequestReportRate(uint8_t seq)
{
    CY_Z_SendCommand(CY_Z_CMD_GET_RATE, 0x00U, seq);
}

/* 设置上报频率，rate 直接用头文件里的枚举值。 */
void CY_Z_SetReportRate(CY_Z_ReportRate rate, uint8_t seq)
{
    CY_Z_SendCommand(CY_Z_CMD_SET_RATE, (uint8_t) rate, seq);
}

/* 查询模式下手动要一帧遥测。 */
void CY_Z_RequestTelemetry(uint8_t seq)
{
    CY_Z_SendCommand(CY_Z_CMD_QUERY_TELEMETRY, 0x00U, seq);
}

/* 发比例因子查询命令；返回帧目前还没解析。 */
void CY_Z_RequestScaleFactor(uint8_t seq)
{
    CY_Z_SendCommand(CY_Z_CMD_GET_SCALE, 0x00U, seq);
}

/* 让模块进 Bootloader，只做到入口命令这一步。 */
void CY_Z_EnterBootloader(uint8_t seq)
{
    CY_Z_SendCommand(CY_Z_CMD_BOOTLOADER, 0x00U, seq);
}

/* UART1 的实际接收处理，应用层不用手动调。 */
void CY_Z_UART1_IRQHandler(void)
{
    uint8_t data;
    uint8_t frame[CY_Z_TELEMETRY_FRAME_SIZE];
    CY_Z_Telemetry telemetry;
    CY_Z_Ack ack;

    switch (DL_UART_Main_getPendingInterrupt(UART_1_INST)) {
        case DL_UART_MAIN_IIDX_RX:
            data = DL_UART_Main_receiveData(UART_1_INST);
            g_rx_byte_count++;

            /* 遥测帧长 16 字节，ACK 只有 8 字节，所以共用一个 16 字节滑动窗口。
             * 每来一个字节先尝试识别末尾的 ACK；凑满 16 字节后再尝试遥测帧。
             * 校验失败时左移 1 字节继续找帧头，这样丢字节后也能重新同步。 */
            if (g_rx_window_count < CY_Z_TELEMETRY_FRAME_SIZE) {
                g_rx_window[g_rx_window_count] = data;
                g_rx_window_count++;
            } else {
                for (uint8_t i = 0U; i < (CY_Z_TELEMETRY_FRAME_SIZE - 1U); i++) {
                    g_rx_window[i] = g_rx_window[i + 1U];
                }
                g_rx_window[CY_Z_TELEMETRY_FRAME_SIZE - 1U] = data;
            }

            if (g_rx_window_count >= CY_Z_ACK_FRAME_SIZE) {
                uint8_t start = g_rx_window_count - CY_Z_ACK_FRAME_SIZE;

                if ((g_rx_window[start] == CY_Z_ACK_HEADER0) &&
                    (g_rx_window[start + 1U] == CY_Z_ACK_HEADER1)) {
                    for (uint8_t i = 0U; i < CY_Z_ACK_FRAME_SIZE; i++) {
                        frame[i] = g_rx_window[start + i];
                    }

                    if (CY_Z_ParseAckFrame(frame, &ack)) {
                        g_last_ack = ack;
                        g_ack_ready = true;
                        g_ack_frame_count++;
                        g_rx_window_count = 0U;
                        break;
                    }
                }
            }

            if (g_rx_window_count >= CY_Z_TELEMETRY_FRAME_SIZE) {
                for (uint8_t i = 0U; i < CY_Z_TELEMETRY_FRAME_SIZE; i++) {
                    frame[i] = g_rx_window[i];
                }

                if (CY_Z_ParseTelemetryFrame(frame, &telemetry)) {
                    g_last_telemetry = telemetry;
                    g_frame_ready = true;
                    g_valid_frame_count++;
                    g_rx_window_count = 0U;
                } else {
                    for (uint8_t i = 0U; i < (CY_Z_TELEMETRY_FRAME_SIZE - 1U); i++) {
                        g_rx_window[i] = g_rx_window[i + 1U];
                    }
                    g_rx_window_count = CY_Z_TELEMETRY_FRAME_SIZE - 1U;
                    g_drop_byte_count++;
                }
            }
            break;

        default:
            break;
    }
}

/* SysConfig 对应的 UART1 中断入口。 */
void UART_1_INST_IRQHandler(void)
{
    /* UART_1_INST_IRQHandler 是 SysConfig 生成的宏，当前会展开为 UART1_IRQHandler。
     * 这个函数就是实际中断向量入口，不要在别处再写一个同名处理函数。 */
    CY_Z_UART1_IRQHandler();
}
