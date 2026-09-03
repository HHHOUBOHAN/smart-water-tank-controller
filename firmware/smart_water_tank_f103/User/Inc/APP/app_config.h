#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t low_level_percent;
    uint8_t high_level_percent;
    int32_t pressure_trip_delta_raw;
    int32_t pressure_recover_delta_raw;
    uint16_t max_run_minutes;
} AppConfigData_t;

typedef enum
{
    APP_CONFIG_ITEM_LOW_LEVEL = 0,
    APP_CONFIG_ITEM_HIGH_LEVEL,
    APP_CONFIG_ITEM_PRESS_TRIP,
    APP_CONFIG_ITEM_PRESS_RECOVER,
    APP_CONFIG_ITEM_MAX_RUN,
    APP_CONFIG_ITEM_COUNT
} AppConfigItem_t;

typedef enum
{
    APP_CONFIG_APPLY_OK = 0,
    APP_CONFIG_APPLY_BUSY,
    APP_CONFIG_APPLY_INVALID,
    APP_CONFIG_APPLY_SAVE_FAILED
} AppConfigApplyResult_t;

void AppConfig_Init(void);
const AppConfigData_t *AppConfig_GetActive(void);
const AppConfigData_t *AppConfig_GetEditing(void);
void AppConfig_BeginEdit(void);
void AppConfig_CancelEdit(void);
bool AppConfig_CommitEdit(void);
void AppConfig_NextItem(void);
void AppConfig_AdjustCurrent(int8_t direction);
AppConfigItem_t AppConfig_GetCurrentItem(void);
bool AppConfig_IsEditing(void);
AppConfigApplyResult_t AppConfig_ApplyRemote(const AppConfigData_t *config);

#endif /* APP_CONFIG_H */
