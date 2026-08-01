#include "grayscale_sensor.h"
#include "app_config.h"
#include <stdbool.h>

uint8_t g_sensor_raw_data = 0xFF;
int16_t cx = LINE_CENTER;

void Grayscale_Sensor_Init(void)
{
    PL_HIGH();
    SCK_LOW();

    DL_GPIO_initDigitalInputFeatures(GPIO_Sensor_PIN_SDA_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
}

static uint8_t Grayscale_Read_All(void)
{
    uint8_t data = 0;

    PL_LOW();
    __asm volatile ("nop");
    PL_HIGH();
    __asm volatile ("nop");

    if (SDA_READ()) {
        data |= (1U << 7);
    }

    for (uint8_t i = 0; i < 7; i++) {
        SCK_HIGH();
        __asm volatile ("nop");
        SCK_LOW();
        __asm volatile ("nop");
        if (SDA_READ()) {
            data |= (uint8_t)(1U << (6U - i));
        }
    }

    return data;
}

uint8_t Grayscale_Update(void)
{
    uint16_t weighted_sum = 0;
    uint8_t black_count = 0;

    g_sensor_raw_data = Grayscale_Read_All();

    /*
     * 通道 0~3 位于车体右侧，通道 4~7 位于左侧。
     * 权重 70~0 的重心坐标与现有电机转向方向保持一致。
     */
    for (uint8_t i = 0; i < GRAYSCALE_SENSOR_CHANNELS; i++) {
        bool level_high =
            ((g_sensor_raw_data & (uint8_t)(1U << i)) != 0U);
        bool is_black = SENSOR_BLACK_IS_LOW ? !level_high : level_high;

        if (is_black) {
            weighted_sum += (uint16_t)((7U - i) * 10U);
            black_count++;
        }
    }

    /*
     * 只有本帧存在有效黑线时才更新位置。主控制器始终运行同一套灰度
     * 算法，不包含丢线搜索、航向保持或直角弯状态。
     */
    if (black_count > 0U) {
        cx = (int16_t)((weighted_sum + (black_count / 2U)) / black_count);
    }

    return black_count;
}
