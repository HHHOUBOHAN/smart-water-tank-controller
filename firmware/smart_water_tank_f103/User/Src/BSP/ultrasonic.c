#include "BSP/ultrasonic.h"

#include "BSP/board.h"
#include "tim.h"

#define ULTRASONIC_TIMEOUT_MS        60U
#define ULTRASONIC_TRIGGER_US        12U
#define ULTRASONIC_MIN_PULSE_US      100U
#define ULTRASONIC_MAX_PULSE_US      40000U
#define ULTRASONIC_TRIGGER_GUARD     10000U

typedef enum
{
    ULTRASONIC_STATE_IDLE = 0,
    ULTRASONIC_STATE_WAIT_RISE,
    ULTRASONIC_STATE_WAIT_FALL
} Ultrasonic_State_t;

static volatile Ultrasonic_State_t ultrasonic_state;
static volatile uint16_t ultrasonic_rise_count;
static volatile uint32_t ultrasonic_started_at_ms;
static Ultrasonic_Data_t ultrasonic_data;

void Ultrasonic_Init(void)
{
    HAL_GPIO_WritePin(BOARD_US_TRIG_GPIO_PORT,
                      BOARD_US_TRIG_GPIO_PIN,
                      GPIO_PIN_RESET);
    (void)HAL_TIM_Base_Start(&htim2);
    __HAL_TIM_SET_COUNTER(&htim2, 0U);

    ultrasonic_state = ULTRASONIC_STATE_IDLE;
    ultrasonic_rise_count = 0U;
    ultrasonic_started_at_ms = 0U;
    ultrasonic_data.valid = false;
    ultrasonic_data.busy = false;
    ultrasonic_data.distance_mm = 0U;
    ultrasonic_data.pulse_width_us = 0U;
    ultrasonic_data.last_update_ms = 0U;
    ultrasonic_data.timeout_count = 0U;
}

bool Ultrasonic_Start(uint32_t now_ms)
{
    uint16_t start_count;
    uint32_t guard_count = 0U;

    if (ultrasonic_state != ULTRASONIC_STATE_IDLE)
    {
        return false;
    }

    ultrasonic_started_at_ms = now_ms;
    ultrasonic_state = ULTRASONIC_STATE_WAIT_RISE;
    ultrasonic_data.busy = true;

    HAL_GPIO_WritePin(BOARD_US_TRIG_GPIO_PORT,
                      BOARD_US_TRIG_GPIO_PIN,
                      GPIO_PIN_SET);
    start_count = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);
    while ((uint16_t)((uint16_t)__HAL_TIM_GET_COUNTER(&htim2) - start_count) <
           (uint16_t)ULTRASONIC_TRIGGER_US)
    {
        /* A stopped or misconfigured TIM2 must never freeze the whole
         * application. The normal path exits by elapsed timer ticks; this
         * guard is only the abnormal fail-safe path. */
        guard_count++;
        if (guard_count >= ULTRASONIC_TRIGGER_GUARD)
        {
            break;
        }
    }
    HAL_GPIO_WritePin(BOARD_US_TRIG_GPIO_PORT,
                      BOARD_US_TRIG_GPIO_PIN,
                      GPIO_PIN_RESET);
    return true;
}

void Ultrasonic_Update(uint32_t now_ms)
{
    if ((ultrasonic_state != ULTRASONIC_STATE_IDLE) &&
        ((uint32_t)(now_ms - ultrasonic_started_at_ms) >= ULTRASONIC_TIMEOUT_MS))
    {
        ultrasonic_state = ULTRASONIC_STATE_IDLE;
        ultrasonic_data.busy = false;
        ultrasonic_data.valid = false;
        ultrasonic_data.timeout_count++;
        ultrasonic_data.last_update_ms = now_ms;
    }
}

void Ultrasonic_HandleExti(uint16_t gpio_pin, uint32_t now_ms)
{
    uint16_t current_count;
    uint16_t pulse_width;

    if (gpio_pin != BOARD_US_ECHO_GPIO_PIN)
    {
        return;
    }

    current_count = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);

    if ((ultrasonic_state == ULTRASONIC_STATE_WAIT_RISE) &&
        (HAL_GPIO_ReadPin(BOARD_US_ECHO_GPIO_PORT,
                          BOARD_US_ECHO_GPIO_PIN) == GPIO_PIN_SET))
    {
        ultrasonic_rise_count = current_count;
        ultrasonic_state = ULTRASONIC_STATE_WAIT_FALL;
    }
    else if ((ultrasonic_state == ULTRASONIC_STATE_WAIT_FALL) &&
             (HAL_GPIO_ReadPin(BOARD_US_ECHO_GPIO_PORT,
                               BOARD_US_ECHO_GPIO_PIN) == GPIO_PIN_RESET))
    {
        pulse_width = (uint16_t)(current_count - ultrasonic_rise_count);
        ultrasonic_data.pulse_width_us = pulse_width;
        ultrasonic_data.distance_mm = ((uint32_t)pulse_width * 10U + 29U) / 58U;
        ultrasonic_data.valid = ((uint32_t)pulse_width >= ULTRASONIC_MIN_PULSE_US) &&
                                ((uint32_t)pulse_width <= ULTRASONIC_MAX_PULSE_US);
        ultrasonic_data.busy = false;
        ultrasonic_data.last_update_ms = now_ms;
        ultrasonic_state = ULTRASONIC_STATE_IDLE;
    }
}

const Ultrasonic_Data_t *Ultrasonic_GetData(void)
{
    return &ultrasonic_data;
}
