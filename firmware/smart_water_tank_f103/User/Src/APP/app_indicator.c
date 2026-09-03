#include "APP/app_indicator.h"

#include "APP/app_alarm.h"
#include "BSP/buzzer.h"
#include "BSP/led.h"
#include "BSP/relay.h"

#define APP_INDICATOR_FAULT_HALF_MS  200U

static uint32_t app_indicator_next_fault_ms;

void AppIndicator_Init(uint32_t now_ms)
{
    app_indicator_next_fault_ms = now_ms;
    LED_Off(LED_RUN);
    LED_Off(LED_FAULT);
    Buzzer_Off();
}

void AppIndicator_Process(uint32_t now_ms)
{
    LED_SetState(LED_RUN, Relay_IsOn() ? LED_STATE_ON : LED_STATE_OFF);

    if (!AppAlarm_HasAny())
    {
        LED_Off(LED_FAULT);
        Buzzer_Off();
        return;
    }

    if (AppAlarm_IsMuted())
    {
        LED_On(LED_FAULT);
        Buzzer_Off();
        return;
    }

    if ((int32_t)(now_ms - app_indicator_next_fault_ms) >= 0)
    {
        app_indicator_next_fault_ms = now_ms + APP_INDICATOR_FAULT_HALF_MS;
        LED_Toggle(LED_FAULT);
    }
    Buzzer_On();
}
