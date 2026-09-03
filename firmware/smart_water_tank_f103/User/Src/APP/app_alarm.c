#include "APP/app_alarm.h"

#define APP_ALARM_SEVERE_MASK ((uint32_t)APP_ALARM_ULTRASONIC | \
                               (uint32_t)APP_ALARM_PRESSURE | \
                               (uint32_t)APP_ALARM_OVERPRESSURE | \
                               (uint32_t)APP_ALARM_MAX_RUNTIME)

static uint32_t app_alarm_flags;
static bool app_alarm_muted;

void AppAlarm_Init(void)
{
    app_alarm_flags = 0U;
    app_alarm_muted = false;
}

void AppAlarm_SetCondition(AppAlarmFlag_t flag, bool active)
{
    const uint32_t mask = (uint32_t)flag;
    const bool was_active = ((app_alarm_flags & mask) != 0U);

    if (active)
    {
        app_alarm_flags |= mask;
        if (!was_active)
        {
            app_alarm_muted = false;
        }
    }
    else
    {
        app_alarm_flags &= ~mask;
        if (app_alarm_flags == 0U)
        {
            app_alarm_muted = false;
        }
    }
}

void AppAlarm_ToggleMute(void)
{
    if (app_alarm_flags != 0U)
    {
        app_alarm_muted = !app_alarm_muted;
    }
}

void AppAlarm_Mute(void)
{
    if (app_alarm_flags != 0U)
    {
        app_alarm_muted = true;
    }
}

bool AppAlarm_IsMuted(void)
{
    return app_alarm_muted;
}

bool AppAlarm_HasAny(void)
{
    return (app_alarm_flags != 0U);
}

bool AppAlarm_HasSevere(void)
{
    return ((app_alarm_flags & APP_ALARM_SEVERE_MASK) != 0U);
}

bool AppAlarm_IsActive(AppAlarmFlag_t flag)
{
    return ((app_alarm_flags & (uint32_t)flag) != 0U);
}

uint32_t AppAlarm_GetFlags(void)
{
    return app_alarm_flags;
}

const char *AppAlarm_GetPrimaryText(void)
{
    if (AppAlarm_IsActive(APP_ALARM_CONFIG))
    {
        return "CONFIG";
    }
    if (AppAlarm_IsActive(APP_ALARM_OVERPRESSURE))
    {
        return "PRESS HIGH";
    }
    if (AppAlarm_IsActive(APP_ALARM_MAX_RUNTIME))
    {
        return "RUN TIME";
    }
    if (AppAlarm_IsActive(APP_ALARM_ULTRASONIC))
    {
        return "US SENSOR";
    }
    if (AppAlarm_IsActive(APP_ALARM_PRESSURE))
    {
        return "P SENSOR";
    }
    if (AppAlarm_IsActive(APP_ALARM_OLED))
    {
        return "OLED";
    }
    return "NONE";
}
