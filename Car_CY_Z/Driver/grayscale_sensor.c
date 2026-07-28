#include "grayscale_sensor.h"
#include "ti_msp_dl_config.h"

uint8_t g_sensor_raw_data = 0xFF;   // 原始数据: 0=黑, 1=白
int16_t cx = 35;
uint8_t Flag        = 0;            // 丢线标志: 0=正常, 1=丢线

void Grayscale_Sensor_Init(void)
{
    PL_HIGH();
    SCK_HIGH();

    // PB13 (SDA) → 输入+上拉
    DL_GPIO_initDigitalInputFeatures(GPIO_Sensor_PIN_SDA_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
}

/* 统计 data 中黑点 (bit=0) 的数量 */
static uint8_t Grayscale_Count_Black(uint8_t data)
{
    uint8_t cnt = 0;
    for (uint8_t i = 0; i < 8; i++)
        if (!(data & (1 << i))) cnt++;
    return cnt;
}

// 74HC165 读取 8 通道
static uint8_t Grayscale_Read_All(void)
{
    uint8_t data = 0;

    PL_LOW();
    __asm volatile ("nop");
    PL_HIGH();
    __asm volatile ("nop");

    if (SDA_READ()) data |= (1 << 7);

    for (uint8_t i = 0; i < 7; i++) {
        SCK_HIGH();
        __asm volatile ("nop");
        SCK_LOW();
        __asm volatile ("nop");
        if (SDA_READ()) data |= (1 << (6 - i));
    }

    return data;
}

// data → cx
// data 中 bit=0 表示黑线, bit=1 表示白
// 物理: 通道0~3=右, 通道4~7=左
// 权重: 右→大(右转), 左→小(左转)
// 通道 i: 权重 = (7-i)*10
static uint16_t Grayscale_Data_To_CX(uint8_t data)
{
    uint16_t sum = 0;
    uint8_t count = 0;

    for (uint8_t i = 0; i < 8; i++) {
        if (!(data & (1 << i))) {       // bit=0 → 黑线
            sum += (uint16_t)((7 - i) * 10);
            count++;
        }
    }

    if (count == 0) return 255;  // 全白(0xFF) → 丢线

    return (uint16_t)((sum + (count / 2)) / count);
}

uint8_t Grayscale_Update(void)
{
    static uint8_t  corner_lock = 0;    // 锁定的转弯方向 (0=无锁)
    static uint16_t lock_cnt    = 0;    // 剩余锁定周期 (每周期≈3ms)

    g_sensor_raw_data = Grayscale_Read_All();

    /* 锁定期间: 强制维持转弯信号，0.5s 内不响应传感器变化 */
    if (corner_lock && lock_cnt > 0) {
        lock_cnt--;
        cx = 35;
        Flag = corner_lock;
        if (g_sensor_raw_data == 0x00) {   // 全黑例外: 立即停车
            Flag = 0; corner_lock = 0; lock_cnt = 0;
        }
        return Flag;
    }
    corner_lock = 0;  // 锁到期，释放

    if (g_sensor_raw_data == 0x00) {
        cx = 35;  Flag = 0;
    } else if (g_sensor_raw_data == 0xFF) {
        cx = 35;  Flag = 3;
    } else if ((g_sensor_raw_data & 0xE0) == 0x00 &&
               Grayscale_Count_Black(g_sensor_raw_data) >= 4) {
        /* 左边3黑 + 黑点≥4 → 左转，锁 0.45s */
        cx = 35;  Flag = 1;  corner_lock = 1;  lock_cnt = 150;
    } else if ((g_sensor_raw_data & 0x07) == 0x00 &&
               Grayscale_Count_Black(g_sensor_raw_data) >= 4) {
        /* 右边3黑 + 黑点≥4 → 右转，锁 0.45s */
        cx = 35;  Flag = 2;  corner_lock = 2;  lock_cnt = 150;
    } else {
        /* 正常循线：紧跟黑线，向 11100111 贴近 */
        cx = Grayscale_Data_To_CX(g_sensor_raw_data);
        Flag = 4;
    }
    return Flag;
}
