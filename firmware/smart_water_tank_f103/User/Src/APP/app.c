#include "APP/app.h"

#include "APP/app_alarm.h"
#include "APP/app_command.h"
#include "APP/app_config.h"
#include "APP/app_control.h"
#include "APP/app_display.h"
#include "APP/app_indicator.h"
#include "APP/app_state.h"
#include "BSP/buzzer.h"
#include "BSP/key.h"
#include "BSP/led.h"
#include "BSP/relay.h"
#include "BSP/ultrasonic.h"
#include "NETWORK/network_manager.h"
#include "SERVICE/sensor_service.h"
#include "usart.h"

#define APP_KEY_SCAN_PERIOD_MS  5U
#define APP_MAX_COMMANDS_PER_RUN 8U

static uint32_t app_next_key_scan_ms;
static uint32_t app_self_test_next_ms;
static uint8_t app_self_test_phase;
static bool app_self_test_active;

static bool App_TimeReached(uint32_t now_ms, uint32_t target_ms)
{
    return ((int32_t)(now_ms - target_ms) >= 0);
}

static void App_StartupSelfTestBegin(uint32_t now_ms)
{
    Relay_Off();
    Buzzer_Off();
    LED_Off(LED_FAULT);
    LED_On(LED_RUN);
    app_self_test_phase = 0U;
    app_self_test_next_ms = now_ms + 150U;
    app_self_test_active = true;
}

static void App_StartupSelfTestProcess(uint32_t now_ms)
{
    if (!app_self_test_active ||
        !App_TimeReached(now_ms, app_self_test_next_ms))
    {
        return;
    }

    switch (app_self_test_phase)
    {
        case 0U: LED_Off(LED_RUN); break;
        case 1U: LED_On(LED_RUN); break;
        case 2U: LED_Off(LED_RUN); break;
        case 3U: LED_On(LED_RUN); break;
        case 4U: LED_Off(LED_RUN); break;
        case 5U:
            LED_On(LED_FAULT);
            Buzzer_On();
            break;
        case 6U:
        default:
            LED_Off(LED_FAULT);
            Buzzer_Off();
            app_self_test_active = false;
            return;
    }

    app_self_test_phase++;
    app_self_test_next_ms = now_ms +
        ((app_self_test_phase == 6U) ? 300U : 150U);
}

void App_Init(void)
{
    uint32_t now_ms;

    Relay_Init();
    Buzzer_Init();
    LED_Init();

    now_ms = HAL_GetTick();
    AppState_Init(now_ms);
    AppConfig_Init();
    AppAlarm_Init();
    AppCommand_Init();
    Key_Init(now_ms);
    SensorService_Init(now_ms);
    AppControl_Init(now_ms);

    /* The V1 transport is enabled so UART reception is ready. No AT/MQTT
     * server values are guessed, therefore the visible state is OFFLINE. */
    NetworkManager_Init(&huart1, true);
    AppDisplay_Init(now_ms);
    AppAlarm_SetCondition(APP_ALARM_OLED, !AppDisplay_IsReady());
    AppIndicator_Init(now_ms);
    App_StartupSelfTestBegin(now_ms);
    AppDisplay_Process(now_ms);

    app_next_key_scan_ms = now_ms;
}

void App_Run(void)
{
    const uint32_t now_ms = HAL_GetTick();
    AppCommand_t command;
    uint8_t processed_commands = 0U;

    /* Scan keys before sensor, network and OLED work. A slow peripheral must
     * not postpone the user input path to the end of the main loop. */
    if (App_TimeReached(now_ms, app_next_key_scan_ms))
    {
        app_next_key_scan_ms = now_ms + APP_KEY_SCAN_PERIOD_MS;
        Key_Update(now_ms);
        AppCommand_Update();
    }

    SensorService_Process(now_ms);
    NetworkManager_Update();

    if (AppCommand_TakeStopLatch())
    {
        command.type = APP_COMMAND_STOP;
        command.source = APP_COMMAND_SOURCE_SYSTEM;
        command.timestamp_ms = now_ms;
        AppControl_HandleCommand(&command, now_ms);
        AppDisplay_RequestRefresh();
    }

    /* Do not remove commands from the queue while the startup lamp test is
     * active. The old code silently consumed them without executing them. */
    while (!app_self_test_active &&
           (processed_commands < APP_MAX_COMMANDS_PER_RUN) &&
           AppCommand_Get(&command))
    {
        AppControl_HandleCommand(&command, now_ms);
        AppDisplay_RequestRefresh();
        processed_commands++;
    }

    App_StartupSelfTestProcess(now_ms);
    if (!app_self_test_active)
    {
        AppControl_Process(now_ms);
        AppIndicator_Process(now_ms);
    }
    AppDisplay_Process(now_ms);
}

void App_OnGpioExti(uint16_t gpio_pin)
{
    Ultrasonic_HandleExti(gpio_pin, HAL_GetTick());
}

void App_OnUartRxComplete(UART_HandleTypeDef *uart)
{
    NetworkManager_OnUartRxComplete(uart);
}

void App_OnUartError(UART_HandleTypeDef *uart)
{
    NetworkManager_OnUartError(uart);
}
