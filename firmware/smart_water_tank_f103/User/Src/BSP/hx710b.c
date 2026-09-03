#include "BSP/hx710b.h"

#include "BSP/board.h"

#define HX710B_FILTER_SHIFT  3U

static HX710B_Data_t hx710b_data;

static void HX710B_ClockDelay(void)
{
    uint32_t count;
    for (count = 0U; count < 12U; count++)
    {
        __NOP();
    }
}

void HX710B_Init(void)
{
    HAL_GPIO_WritePin(BOARD_PRESS_SCK_GPIO_PORT,
                      BOARD_PRESS_SCK_GPIO_PIN,
                      GPIO_PIN_RESET);
    hx710b_data.valid = false;
    hx710b_data.raw = 0;
    hx710b_data.filtered_raw = 0;
    hx710b_data.zero_offset = 0;
    hx710b_data.sample_count = 0U;
}

bool HX710B_IsReady(void)
{
    return (HAL_GPIO_ReadPin(BOARD_PRESS_OUT_GPIO_PORT,
                             BOARD_PRESS_OUT_GPIO_PIN) == GPIO_PIN_RESET);
}

HX710B_Status_t HX710B_TryRead(int32_t *raw_value)
{
    uint32_t bits = 0U;
    uint32_t index;
    uint32_t primask;

    if (raw_value == 0)
    {
        return HX710B_STATUS_INVALID_ARGUMENT;
    }
    if (!HX710B_IsReady())
    {
        return HX710B_STATUS_NOT_READY;
    }

    primask = __get_PRIMASK();
    __disable_irq();

    for (index = 0U; index < 24U; index++)
    {
        HAL_GPIO_WritePin(BOARD_PRESS_SCK_GPIO_PORT,
                          BOARD_PRESS_SCK_GPIO_PIN,
                          GPIO_PIN_SET);
        HX710B_ClockDelay();
        bits = (bits << 1U) |
               ((HAL_GPIO_ReadPin(BOARD_PRESS_OUT_GPIO_PORT,
                                  BOARD_PRESS_OUT_GPIO_PIN) == GPIO_PIN_SET) ? 1U : 0U);
        HAL_GPIO_WritePin(BOARD_PRESS_SCK_GPIO_PORT,
                          BOARD_PRESS_SCK_GPIO_PIN,
                          GPIO_PIN_RESET);
        HX710B_ClockDelay();
    }

    /* The 25th clock completes the conversion read and selects the normal
     * differential-input conversion for the next sample. */
    HAL_GPIO_WritePin(BOARD_PRESS_SCK_GPIO_PORT,
                      BOARD_PRESS_SCK_GPIO_PIN,
                      GPIO_PIN_SET);
    HX710B_ClockDelay();
    HAL_GPIO_WritePin(BOARD_PRESS_SCK_GPIO_PORT,
                      BOARD_PRESS_SCK_GPIO_PIN,
                      GPIO_PIN_RESET);

    if (primask == 0U)
    {
        __enable_irq();
    }

    if ((bits & 0x00800000UL) != 0U)
    {
        bits |= 0xFF000000UL;
    }
    *raw_value = (int32_t)bits;
    return HX710B_STATUS_OK;
}

void HX710B_AddSample(int32_t raw_value)
{
    hx710b_data.raw = raw_value;
    if (!hx710b_data.valid)
    {
        hx710b_data.filtered_raw = raw_value;
        hx710b_data.valid = true;
    }
    else
    {
        hx710b_data.filtered_raw +=
            (raw_value - hx710b_data.filtered_raw) >> HX710B_FILTER_SHIFT;
    }
    hx710b_data.sample_count++;
}

void HX710B_Tare(void)
{
    if (hx710b_data.valid)
    {
        hx710b_data.zero_offset = hx710b_data.filtered_raw;
    }
}

const HX710B_Data_t *HX710B_GetData(void)
{
    return &hx710b_data;
}
