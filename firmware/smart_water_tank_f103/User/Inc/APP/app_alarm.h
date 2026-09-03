#ifndef APP_ALARM_H
#define APP_ALARM_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    APP_ALARM_NONE          = 0U,
    APP_ALARM_ULTRASONIC    = (1U << 0),
    APP_ALARM_PRESSURE      = (1U << 1),
    APP_ALARM_OVERPRESSURE  = (1U << 2),
    APP_ALARM_MAX_RUNTIME   = (1U << 3),
    APP_ALARM_CONFIG        = (1U << 4),
    APP_ALARM_OLED          = (1U << 5)
} AppAlarmFlag_t;

void AppAlarm_Init(void);
void AppAlarm_SetCondition(AppAlarmFlag_t flag, bool active);
void AppAlarm_ToggleMute(void);
void AppAlarm_Mute(void);
bool AppAlarm_IsMuted(void);
bool AppAlarm_HasAny(void);
bool AppAlarm_HasSevere(void);
bool AppAlarm_IsActive(AppAlarmFlag_t flag);
uint32_t AppAlarm_GetFlags(void);
const char *AppAlarm_GetPrimaryText(void);

#endif /* APP_ALARM_H */
