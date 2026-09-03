#include "APP/app_control.h"

#include "APP/app_alarm.h"
#include "APP/app_config.h"
#include "APP/app_state.h"
#include "BSP/relay.h"
#include "SERVICE/sensor_service.h"

#include <stdbool.h>
#include <string.h>

#define APP_CONTROL_SENSOR_GRACE_MS   5000U
#define APP_CONTROL_LOW_CONFIRM_MS    3000U
#define APP_CONTROL_RECOVERY_MS       3000U
#define APP_CONTROL_NOTICE_MS         2000U

typedef enum
{
    APP_CONTROL_EVENT_NONE        = 0U,
    APP_CONTROL_EVENT_STOP        = (1U << 0),
    APP_CONTROL_EVENT_START       = (1U << 1),
    APP_CONTROL_EVENT_MODE_TOGGLE = (1U << 2)
} AppControlEvent_t;

static uint32_t app_control_started_ms;
static uint32_t app_control_low_since_ms;
static uint32_t app_control_recovery_since_ms;
static uint32_t app_control_events;
static bool app_control_low_timing;
static bool app_control_recovery_timing;
static bool app_control_timeout_latched;
static const char *app_control_notice_text;
static uint32_t app_control_notice_until_ms;

static void AppControl_SetNotice(const char *text, uint32_t now_ms)
{
    app_control_notice_text = text;
    app_control_notice_until_ms = now_ms + APP_CONTROL_NOTICE_MS;
}

static bool AppControl_EventPending(AppControlEvent_t event)
{
    return ((app_control_events & (uint32_t)event) != 0U);
}

static void AppControl_ClearEvent(AppControlEvent_t event)
{
    app_control_events &= ~(uint32_t)event;
}

static void AppControl_SetPump(bool on, uint32_t now_ms)
{
    if (on)
    {
        Relay_On();
        AppState_SetPumpCommand(true, now_ms);
        AppState_SetRunState(APP_RUN_RUNNING);
    }
    else
    {
        Relay_Off();
        AppState_SetPumpCommand(false, now_ms);
    }
}

bool AppControl_CanStart(void)
{
    const SensorServiceData_t *sensors = SensorService_GetData();
    const AppConfigData_t *config = AppConfig_GetActive();

    return sensors->all_valid && !AppAlarm_HasSevere() &&
           (AppState_GetRunState() != APP_RUN_FAULT_LOCKED) &&
           (sensors->level_percent < config->high_level_percent) &&
           (sensors->pressure_delta_raw <
            config->pressure_trip_delta_raw);
}

const char *AppControl_GetStartDeniedReason(void)
{
    const SensorServiceData_t *sensors = SensorService_GetData();
    const AppConfigData_t *config = AppConfig_GetActive();

    if (!sensors->all_valid)
    {
        return "SENSOR_INVALID";
    }
    if (AppState_GetRunState() == APP_RUN_FAULT_LOCKED)
    {
        return "FAULT_LOCKED";
    }
    if (sensors->level_percent >= config->high_level_percent)
    {
        return "HIGH_LEVEL";
    }
    if (sensors->pressure_delta_raw >= config->pressure_trip_delta_raw)
    {
        return "PRESSURE_HIGH";
    }
    return "FAULT_LOCKED";
}

static const char *AppControl_StartDeniedText(void)
{
    const char *reason = AppControl_GetStartDeniedReason();

    if (strcmp(reason, "SENSOR_INVALID") == 0) return "START DENIED:SENSOR";
    if (strcmp(reason, "HIGH_LEVEL") == 0)     return "START DENIED:HIGH";
    if (strcmp(reason, "PRESSURE_HIGH") == 0) return "START DENIED:PRESS";
    return "START DENIED:FAULT";
}

void AppControl_Init(uint32_t now_ms)
{
    app_control_started_ms = now_ms;
    app_control_low_since_ms = 0U;
    app_control_recovery_since_ms = 0U;
    app_control_events = APP_CONTROL_EVENT_NONE;
    app_control_low_timing = false;
    app_control_recovery_timing = false;
    app_control_timeout_latched = false;
    app_control_notice_text = 0;
    app_control_notice_until_ms = 0U;
    AppControl_SetPump(false, now_ms);
    AppState_SetRunState(APP_RUN_BOOT_WAIT_VALID);
}

void AppControl_HandleCommand(const AppCommand_t *command, uint32_t now_ms)
{
    if (command == 0)
    {
        return;
    }

    switch (command->type)
    {
        case APP_COMMAND_STOP:
            app_control_events |= APP_CONTROL_EVENT_STOP;
            app_control_events &= ~(uint32_t)APP_CONTROL_EVENT_START;
            break;

        case APP_COMMAND_START:
            app_control_events |= APP_CONTROL_EVENT_START;
            break;

        case APP_COMMAND_MODE_TOGGLE:
            app_control_events |= APP_CONTROL_EVENT_MODE_TOGGLE;
            break;

        case APP_COMMAND_MUTE_TOGGLE:
            AppAlarm_ToggleMute();
            break;

        case APP_COMMAND_ENTER_CONFIG:
            AppConfig_BeginEdit();
            AppState_SetPage(APP_PAGE_CONFIG);
            break;

        case APP_COMMAND_CONFIG_NEXT:
            AppConfig_NextItem();
            break;

        case APP_COMMAND_CONFIG_INCREASE:
            AppConfig_AdjustCurrent(1);
            break;

        case APP_COMMAND_CONFIG_DECREASE:
            AppConfig_AdjustCurrent(-1);
            break;

        case APP_COMMAND_CONFIG_SAVE:
            if (AppConfig_CommitEdit())
            {
                AppAlarm_SetCondition(APP_ALARM_CONFIG, false);
                AppState_SetPage(APP_PAGE_MAIN);
            }
            else
            {
                AppAlarm_SetCondition(APP_ALARM_CONFIG, true);
                AppControl_SetNotice("SAVE FAIL", now_ms);
            }
            break;

        case APP_COMMAND_CONFIG_CANCEL:
            AppConfig_CancelEdit();
            AppAlarm_SetCondition(APP_ALARM_CONFIG, false);
            AppState_SetPage(APP_PAGE_MAIN);
            break;

        case APP_COMMAND_PRESSURE_TARE:
            SensorService_CapturePressureBaseline();
            AppAlarm_SetCondition(APP_ALARM_OVERPRESSURE, false);
            AppControl_SetPump(false, now_ms);
            AppControl_SetNotice("PRESS ZEROED", now_ms);
            break;

        default:
            break;
    }
}

static void AppControl_UpdateAlarmConditions(uint32_t now_ms)
{
    const SensorServiceData_t *sensors = SensorService_GetData();
    const AppConfigData_t *config = AppConfig_GetActive();
    const bool grace_elapsed =
        ((uint32_t)(now_ms - app_control_started_ms) >=
         APP_CONTROL_SENSOR_GRACE_MS);

    AppAlarm_SetCondition(APP_ALARM_ULTRASONIC,
                          grace_elapsed && !sensors->ultrasonic_valid);
    AppAlarm_SetCondition(APP_ALARM_PRESSURE,
                          grace_elapsed && !sensors->pressure_valid);

    if (sensors->pressure_valid)
    {
        if (sensors->pressure_delta_raw >=
            config->pressure_trip_delta_raw)
        {
            AppAlarm_SetCondition(APP_ALARM_OVERPRESSURE, true);
        }
        else if (sensors->pressure_delta_raw <=
                 config->pressure_recover_delta_raw)
        {
            AppAlarm_SetCondition(APP_ALARM_OVERPRESSURE, false);
        }
    }
    else
    {
        AppAlarm_SetCondition(APP_ALARM_OVERPRESSURE, false);
    }

    if (Relay_IsOn() &&
        ((uint32_t)(now_ms - AppState_GetPumpStartedAtMs()) >=
         ((uint32_t)config->max_run_minutes * 60000U)))
    {
        app_control_timeout_latched = true;
        AppAlarm_SetCondition(APP_ALARM_MAX_RUNTIME, true);
    }
}

static void AppControl_ProcessAuto(uint32_t now_ms)
{
    const SensorServiceData_t *sensors = SensorService_GetData();
    const AppConfigData_t *config = AppConfig_GetActive();

    if (AppState_GetRunState() == APP_RUN_AUTO_PAUSED)
    {
        app_control_low_timing = false;
        return;
    }

    if (Relay_IsOn())
    {
        if (sensors->level_percent >= config->high_level_percent)
        {
            AppControl_SetPump(false, now_ms);
            AppState_SetRunState(APP_RUN_IDLE);
        }
        return;
    }

    if (AppState_GetRunState() != APP_RUN_IDLE)
    {
        return;
    }

    if (sensors->level_percent <= config->low_level_percent)
    {
        if (!app_control_low_timing)
        {
            app_control_low_timing = true;
            app_control_low_since_ms = now_ms;
        }
        else if (((uint32_t)(now_ms - app_control_low_since_ms) >=
                  APP_CONTROL_LOW_CONFIRM_MS) &&
                 AppControl_CanStart())
        {
            AppControl_SetPump(true, now_ms);
            app_control_low_timing = false;
        }
    }
    else
    {
        app_control_low_timing = false;
    }
}

static void AppControl_ProcessStop(uint32_t now_ms)
{
    AppControl_ClearEvent(APP_CONTROL_EVENT_STOP);
    AppControl_ClearEvent(APP_CONTROL_EVENT_START);
    AppControl_SetPump(false, now_ms);
    AppState_SetRunState((AppState_GetMode() == APP_MODE_AUTO) ?
                         APP_RUN_AUTO_PAUSED : APP_RUN_IDLE);
    app_control_low_timing = false;
    AppControl_SetNotice("STOPPED", now_ms);
}

static void AppControl_ProcessModeToggle(uint32_t now_ms)
{
    AppControl_ClearEvent(APP_CONTROL_EVENT_MODE_TOGGLE);
    AppControl_SetPump(false, now_ms);
    app_control_low_timing = false;

    if (AppState_GetMode() == APP_MODE_AUTO)
    {
        AppState_SetMode(APP_MODE_MANUAL);
        AppState_SetRunState(APP_RUN_IDLE);
        AppControl_SetNotice("MODE:MANUAL", now_ms);
    }
    else
    {
        AppState_SetMode(APP_MODE_AUTO);
        AppState_SetRunState(SensorService_GetData()->all_valid ?
                             APP_RUN_IDLE : APP_RUN_BOOT_WAIT_VALID);
        AppControl_SetNotice("MODE:AUTO", now_ms);
    }
}

void AppControl_Process(uint32_t now_ms)
{
    const SensorServiceData_t *sensors = SensorService_GetData();
    const AppConfigData_t *config = AppConfig_GetActive();

    if ((app_control_notice_text != 0) &&
        ((int32_t)(now_ms - app_control_notice_until_ms) >= 0))
    {
        app_control_notice_text = 0;
    }

    AppControl_UpdateAlarmConditions(now_ms);

    /* STOP is always the highest-priority event. */
    if (AppControl_EventPending(APP_CONTROL_EVENT_STOP))
    {
        AppControl_ProcessStop(now_ms);
        return;
    }

    /* START acknowledges a latched runtime timeout, but cannot bypass any
     * other currently active severe condition. */
    if (app_control_timeout_latched &&
        AppControl_EventPending(APP_CONTROL_EVENT_START))
    {
        app_control_timeout_latched = false;
        AppAlarm_SetCondition(APP_ALARM_MAX_RUNTIME, false);
        AppControl_ClearEvent(APP_CONTROL_EVENT_START);
        AppControl_SetPump(false, now_ms);
        AppState_SetRunState((AppState_GetMode() == APP_MODE_AUTO) ?
                             APP_RUN_AUTO_PAUSED : APP_RUN_IDLE);
        AppControl_SetNotice("TIMEOUT ACK", now_ms);
        return;
    }

    /* Mode selection is a user-interface operation and must remain usable
     * while a severe alarm is active. The pump is still forced off below and
     * the run state remains FAULT_LOCKED; changing mode never bypasses safety. */
    if (AppControl_EventPending(APP_CONTROL_EVENT_MODE_TOGGLE))
    {
        AppControl_ProcessModeToggle(now_ms);
        if (!AppAlarm_HasSevere())
        {
            return;
        }
    }

    if (AppAlarm_HasSevere())
    {
        AppControl_SetPump(false, now_ms);
        /* Keep CONFIG accessible during a fault so the operator can inspect
         * and correct thresholds. Entering CONFIG never enables the pump. */
        AppState_SetRunState(APP_RUN_FAULT_LOCKED);
        app_control_recovery_timing = false;
        app_control_low_timing = false;
        return;
    }

    if (AppState_GetRunState() == APP_RUN_FAULT_LOCKED)
    {
        if (!app_control_recovery_timing)
        {
            app_control_recovery_timing = true;
            app_control_recovery_since_ms = now_ms;
        }
        else if ((uint32_t)(now_ms - app_control_recovery_since_ms) >=
                 APP_CONTROL_RECOVERY_MS)
        {
            app_control_recovery_timing = false;
            AppState_SetRunState(APP_RUN_IDLE);
            AppControl_SetNotice("FAULT RECOVERED", now_ms);
        }
        return;
    }

    if (AppState_GetRunState() == APP_RUN_BOOT_WAIT_VALID)
    {
        if (sensors->all_valid)
        {
            AppState_SetRunState(APP_RUN_IDLE);
            AppControl_SetNotice("SENSORS READY", now_ms);
        }
        else
        {
            return;
        }
    }

    if (AppControl_EventPending(APP_CONTROL_EVENT_START))
    {
        AppControl_ClearEvent(APP_CONTROL_EVENT_START);
        if (!AppControl_CanStart())
        {
            AppControl_SetNotice(AppControl_StartDeniedText(), now_ms);
        }
        else if (AppState_GetMode() == APP_MODE_MANUAL)
        {
            AppControl_SetPump(true, now_ms);
        }
        else
        {
            AppState_SetRunState(APP_RUN_IDLE);
            app_control_low_timing = false;
            AppControl_SetNotice("AUTO ACTIVE", now_ms);
        }
    }

    /* High level is a normal completion in both modes, not an alarm. */
    if (Relay_IsOn() &&
        (sensors->level_percent >= config->high_level_percent))
    {
        AppControl_SetPump(false, now_ms);
        AppState_SetRunState(APP_RUN_IDLE);
        app_control_low_timing = false;
        return;
    }

    if ((AppState_GetMode() == APP_MODE_AUTO) && sensors->all_valid)
    {
        AppControl_ProcessAuto(now_ms);
    }
}

const char *AppControl_GetNoticeText(void)
{
    return app_control_notice_text;
}
