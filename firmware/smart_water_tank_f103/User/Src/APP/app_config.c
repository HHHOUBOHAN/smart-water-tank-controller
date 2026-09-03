#include "APP/app_config.h"

#include "SERVICE/storage_service.h"

#include <string.h>

#define APP_CONFIG_DEFAULT_LOW_PERCENT       30U
#define APP_CONFIG_DEFAULT_HIGH_PERCENT      80U
#define APP_CONFIG_DEFAULT_PRESS_TRIP_RAW    1000000L
#define APP_CONFIG_DEFAULT_PRESS_RECOVER_RAW 500000L
#define APP_CONFIG_DEFAULT_MAX_RUN_MIN       30U

static AppConfigData_t app_config_active;
static AppConfigData_t app_config_editing;
static AppConfigItem_t app_config_item;
static bool app_config_is_editing;

static bool AppConfig_IsValid(const AppConfigData_t *config)
{
    if (config == 0)
    {
        return false;
    }
    if ((config->low_level_percent > 95U) ||
        (config->high_level_percent > 100U) ||
        ((uint32_t)config->high_level_percent <
         ((uint32_t)config->low_level_percent + 5U)))
    {
        return false;
    }
    if ((config->pressure_recover_delta_raw < 0L) ||
        (config->pressure_trip_delta_raw <=
         config->pressure_recover_delta_raw) ||
        (config->max_run_minutes == 0U))
    {
        return false;
    }
    return true;
}

void AppConfig_Init(void)
{
    AppConfigData_t saved;

    app_config_active.low_level_percent = APP_CONFIG_DEFAULT_LOW_PERCENT;
    app_config_active.high_level_percent = APP_CONFIG_DEFAULT_HIGH_PERCENT;
    app_config_active.pressure_trip_delta_raw =
        APP_CONFIG_DEFAULT_PRESS_TRIP_RAW;
    app_config_active.pressure_recover_delta_raw =
        APP_CONFIG_DEFAULT_PRESS_RECOVER_RAW;
    app_config_active.max_run_minutes = APP_CONFIG_DEFAULT_MAX_RUN_MIN;
    StorageService_Init();
    if (StorageService_Load(&saved, (uint16_t)sizeof(saved)) &&
        AppConfig_IsValid(&saved))
    {
        app_config_active = saved;
    }
    app_config_editing = app_config_active;
    app_config_item = APP_CONFIG_ITEM_LOW_LEVEL;
    app_config_is_editing = false;
}

const AppConfigData_t *AppConfig_GetActive(void)
{
    return &app_config_active;
}

const AppConfigData_t *AppConfig_GetEditing(void)
{
    return &app_config_editing;
}

void AppConfig_BeginEdit(void)
{
    app_config_editing = app_config_active;
    app_config_item = APP_CONFIG_ITEM_LOW_LEVEL;
    app_config_is_editing = true;
}

void AppConfig_CancelEdit(void)
{
    app_config_editing = app_config_active;
    app_config_is_editing = false;
}

bool AppConfig_CommitEdit(void)
{
    if (!app_config_is_editing || !AppConfig_IsValid(&app_config_editing))
    {
        return false;
    }
    if (!StorageService_Save(&app_config_editing,
                             (uint16_t)sizeof(app_config_editing)))
    {
        return false;
    }
    app_config_active = app_config_editing;
    app_config_is_editing = false;
    return true;
}

void AppConfig_NextItem(void)
{
    if (!app_config_is_editing)
    {
        return;
    }
    app_config_item = (AppConfigItem_t)(((uint32_t)app_config_item + 1U) %
                                        (uint32_t)APP_CONFIG_ITEM_COUNT);
}

void AppConfig_AdjustCurrent(int8_t direction)
{
    if (!app_config_is_editing || (direction == 0))
    {
        return;
    }

    switch (app_config_item)
    {
        case APP_CONFIG_ITEM_LOW_LEVEL:
            if ((direction > 0) && (app_config_editing.low_level_percent < 95U))
            {
                app_config_editing.low_level_percent++;
            }
            else if ((direction < 0) &&
                     (app_config_editing.low_level_percent > 0U))
            {
                app_config_editing.low_level_percent--;
            }
            break;

        case APP_CONFIG_ITEM_HIGH_LEVEL:
            if ((direction > 0) && (app_config_editing.high_level_percent < 100U))
            {
                app_config_editing.high_level_percent++;
            }
            else if ((direction < 0) &&
                     (app_config_editing.high_level_percent > 0U))
            {
                app_config_editing.high_level_percent--;
            }
            break;

        case APP_CONFIG_ITEM_PRESS_TRIP:
            if (direction > 0)
            {
                app_config_editing.pressure_trip_delta_raw += 10000L;
            }
            else if (app_config_editing.pressure_trip_delta_raw >= 10000L)
            {
                app_config_editing.pressure_trip_delta_raw -= 10000L;
            }
            break;

        case APP_CONFIG_ITEM_PRESS_RECOVER:
            if (direction > 0)
            {
                app_config_editing.pressure_recover_delta_raw += 10000L;
            }
            else if (app_config_editing.pressure_recover_delta_raw >= 10000L)
            {
                app_config_editing.pressure_recover_delta_raw -= 10000L;
            }
            break;

        case APP_CONFIG_ITEM_MAX_RUN:
            if ((direction > 0) && (app_config_editing.max_run_minutes < 1440U))
            {
                app_config_editing.max_run_minutes++;
            }
            else if ((direction < 0) &&
                     (app_config_editing.max_run_minutes > 1U))
            {
                app_config_editing.max_run_minutes--;
            }
            break;

        default:
            break;
    }
}

AppConfigItem_t AppConfig_GetCurrentItem(void)
{
    return app_config_item;
}

bool AppConfig_IsEditing(void)
{
    return app_config_is_editing;
}

AppConfigApplyResult_t AppConfig_ApplyRemote(const AppConfigData_t *config)
{
    if (app_config_is_editing)
    {
        return APP_CONFIG_APPLY_BUSY;
    }
    if (!AppConfig_IsValid(config))
    {
        return APP_CONFIG_APPLY_INVALID;
    }
    if (!StorageService_Save(config, (uint16_t)sizeof(*config)))
    {
        return APP_CONFIG_APPLY_SAVE_FAILED;
    }
    app_config_active = *config;
    app_config_editing = *config;
    return APP_CONFIG_APPLY_OK;
}
