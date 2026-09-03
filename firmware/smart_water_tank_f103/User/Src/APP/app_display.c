#include "APP/app_display.h"

#include "APP/app_alarm.h"
#include "APP/app_config.h"
#include "APP/app_control.h"
#include "APP/app_state.h"
#include "BSP/relay.h"
#include "BSP/ssd1306.h"
#include "BSP/ssd1306_fonts.h"
#include "NETWORK/network_manager.h"
#include "SERVICE/sensor_service.h"
#include "i2c.h"
#include "main.h"

#include <stdio.h>
#include <string.h>

#define APP_DISPLAY_I2C_ADDRESS  (0x3CU << 1U)
#define APP_DISPLAY_PERIOD_MS    500U
#define APP_DISPLAY_RETRY_MS     2000U
#define APP_DISPLAY_MAX_FAILURES 2U
#define APP_DISPLAY_FRAME_TIMEOUT_MS 1000U
#define APP_DISPLAY_SUCCESS_WATCHDOG_MS 2000U

static bool app_display_ready;
static bool app_display_transfer_active;
static uint8_t app_display_failure_count;
static uint32_t app_display_next_ms;
static uint32_t app_display_retry_ms;
static bool app_display_refresh_requested;
static uint32_t app_display_transfer_started_ms;
static uint32_t app_display_last_success_ms;

static void AppDisplay_DrawAt(uint8_t x,
                              uint8_t y,
                              const char *text,
                              SSD1306_Font_t font,
                              size_t max_chars)
{
    char buffer[22];
    size_t copy_length = max_chars;

    if (copy_length >= sizeof(buffer))
    {
        copy_length = sizeof(buffer) - 1U;
    }
    (void)memset(buffer, 0, sizeof(buffer));
    if (text != 0)
    {
        (void)strncpy(buffer, text, copy_length);
        buffer[copy_length] = '\0';
    }
    ssd1306_SetCursor(x, y);
    (void)ssd1306_WriteString(buffer, font, White);
}

static void AppDisplay_DrawLine(uint8_t row, const char *text)
{
    char line[22];

    (void)memset(line, ' ', sizeof(line));
    line[21] = '\0';
    if (text != 0)
    {
        (void)strncpy(line, text, 21U);
        line[21] = '\0';
    }
    ssd1306_SetCursor(0U, (uint8_t)(row * 8U));
    (void)ssd1306_WriteString(line, Font_6x8, White);
}

static void AppDisplay_DrawCentered(uint8_t y,
                                    const char *text,
                                    SSD1306_Font_t font)
{
    size_t length = (text == 0) ? 0U : strlen(text);
    uint32_t width = (uint32_t)length * font.width;
    uint8_t x = (width < SSD1306_WIDTH) ?
                (uint8_t)((SSD1306_WIDTH - width) / 2U) : 0U;

    AppDisplay_DrawAt(x, y, text, font, 11U);
}

static void AppDisplay_DrawRight(uint8_t y,
                                 const char *text,
                                 SSD1306_Font_t font,
                                 size_t max_chars)
{
    size_t length = (text == 0) ? 0U : strlen(text);
    uint32_t width;
    uint8_t x;

    if (length > max_chars)
    {
        length = max_chars;
    }
    width = (uint32_t)length * font.width;
    x = (width < SSD1306_WIDTH) ?
        (uint8_t)(SSD1306_WIDTH - width) : 0U;
    AppDisplay_DrawAt(x, y, text, font, max_chars);
}

static const char *AppDisplay_ModeText(void)
{
    return (AppState_GetMode() == APP_MODE_AUTO) ? "AUTO" : "MANUAL";
}

static const char *AppDisplay_StateText(void)
{
    switch (AppState_GetRunState())
    {
        case APP_RUN_BOOT_WAIT_VALID: return "BOOT";
        case APP_RUN_IDLE:            return "IDLE";
        case APP_RUN_RUNNING:         return "RUN";
        case APP_RUN_AUTO_PAUSED:     return "PAUSED";
        case APP_RUN_FAULT_LOCKED:    return "FAULT";
        default:                      return "UNKNOWN";
    }
}

static const char *AppDisplay_NetworkShortText(void)
{
    return NetworkManager_GetDisplayCode();
}

static const char *AppDisplay_AlarmShortText(void)
{
    if (AppAlarm_IsActive(APP_ALARM_CONFIG))       return "CFG";
    if (AppAlarm_IsActive(APP_ALARM_OVERPRESSURE)) return "PHIGH";
    if (AppAlarm_IsActive(APP_ALARM_MAX_RUNTIME))  return "TIME";
    if (AppAlarm_IsActive(APP_ALARM_ULTRASONIC))   return "US";
    if (AppAlarm_IsActive(APP_ALARM_PRESSURE))     return "PRESS";
    if (AppAlarm_IsActive(APP_ALARM_OLED))         return "OLED";
    return "NONE";
}

static void AppDisplay_DrawMain(void)
{
    const SensorServiceData_t *sensors = SensorService_GetData();
    const char *notice = AppControl_GetNoticeText();
    char line[22];
    char left[10];
    char right[10];
    const char *alarm_text;

    AppDisplay_DrawAt(0U, 0U, AppDisplay_ModeText(), Font_6x8, 6U);
    AppDisplay_DrawCentered(0U, AppDisplay_StateText(), Font_6x8);
    (void)snprintf(line, sizeof(line), "P:%s",
                   Relay_IsOn() ? "ON" : "OFF");
    AppDisplay_DrawRight(0U, line, Font_6x8, 6U);

    ssd1306_Line(0U, 9U, 127U, 9U, White);
    ssd1306_Line(63U, 10U, 63U, 53U, White);
    ssd1306_Line(0U, 54U, 127U, 54U, White);
    AppDisplay_DrawAt(8U, 12U, "LEVEL", Font_7x10, 7U);
    AppDisplay_DrawAt(75U, 12U, "P-RAW", Font_7x10, 6U);

    if (sensors->ultrasonic_valid)
    {
        (void)snprintf(left, sizeof(left), "%u%%",
                       (unsigned int)sensors->level_percent);
    }
    else
    {
        (void)snprintf(left, sizeof(left), "---");
    }
    AppDisplay_DrawAt(8U, 28U, left, Font_11x18, 5U);

    if (sensors->pressure_valid)
    {
        (void)snprintf(right, sizeof(right), "%ld",
                       (long)sensors->pressure_delta_raw);
        AppDisplay_DrawAt(68U, 30U, right, Font_7x10, 8U);
    }
    else
    {
        (void)snprintf(right, sizeof(right), "WAIT");
        AppDisplay_DrawAt(68U, 30U, right, Font_7x10, 8U);
    }

    if (notice != 0)
    {
        (void)snprintf(line, sizeof(line), "%s", notice);
    }
    else
    {
        alarm_text = AppDisplay_AlarmShortText();
        if (NetworkManager_IsPublishing())
        {
            (void)snprintf(line, sizeof(line), "N:%s",
                           NetworkManager_GetPublishTrace());
        }
        else if (NetworkManager_GetState() == NETWORK_STATE_RETRY_WAIT)
        {
            (void)snprintf(line, sizeof(line), "N:%s",
                           NetworkManager_GetLastError());
        }
        else
        {
            (void)snprintf(line, sizeof(line), "ALM:%s", alarm_text);
        }
        AppDisplay_DrawAt(0U, 56U, line, Font_6x8, 10U);
        (void)snprintf(line, sizeof(line), "4G:%s",
                       AppDisplay_NetworkShortText());
        AppDisplay_DrawRight(56U, line, Font_6x8, 7U);
        return;
    }
    AppDisplay_DrawAt(0U, 56U, line, Font_6x8, 21U);
}

static const char *AppDisplay_ConfigName(AppConfigItem_t item)
{
    switch (item)
    {
        case APP_CONFIG_ITEM_LOW_LEVEL:     return "LOW LEVEL";
        case APP_CONFIG_ITEM_HIGH_LEVEL:    return "HIGH LEVEL";
        case APP_CONFIG_ITEM_PRESS_TRIP:    return "P STOP RAW";
        case APP_CONFIG_ITEM_PRESS_RECOVER: return "P REC RAW";
        case APP_CONFIG_ITEM_MAX_RUN:       return "MAX RUN";
        default:                            return "UNKNOWN";
    }
}

static void AppDisplay_DrawConfig(void)
{
    const AppConfigData_t *config = AppConfig_GetEditing();
    const AppConfigItem_t item = AppConfig_GetCurrentItem();
    char line[22];
    char value[12];

    (void)snprintf(line, sizeof(line), "CONFIG %u/%u",
                   (unsigned int)item + 1U,
                   (unsigned int)APP_CONFIG_ITEM_COUNT);
    AppDisplay_DrawLine(0U, line);
    ssd1306_Line(0U, 9U, 127U, 9U, White);
    AppDisplay_DrawCentered(13U, AppDisplay_ConfigName(item), Font_7x10);

    switch (item)
    {
        case APP_CONFIG_ITEM_LOW_LEVEL:
            (void)snprintf(value, sizeof(value), "%u%%",
                           (unsigned int)config->low_level_percent);
            break;
        case APP_CONFIG_ITEM_HIGH_LEVEL:
            (void)snprintf(value, sizeof(value), "%u%%",
                           (unsigned int)config->high_level_percent);
            break;
        case APP_CONFIG_ITEM_PRESS_TRIP:
            (void)snprintf(value, sizeof(value), "%ld",
                           (long)config->pressure_trip_delta_raw);
            break;
        case APP_CONFIG_ITEM_PRESS_RECOVER:
            (void)snprintf(value, sizeof(value), "%ld",
                           (long)config->pressure_recover_delta_raw);
            break;
        case APP_CONFIG_ITEM_MAX_RUN:
            (void)snprintf(value, sizeof(value), "%umin",
                           (unsigned int)config->max_run_minutes);
            break;
        default:
            (void)snprintf(value, sizeof(value), "?");
            break;
    }
    AppDisplay_DrawCentered(29U, value, Font_11x18);
    ssd1306_Line(0U, 54U, 127U, 54U, White);
    AppDisplay_DrawAt(0U, 56U, "START:+", Font_6x8, 7U);
    AppDisplay_DrawRight(56U, "STOP:-", Font_6x8, 6U);
}

static void AppDisplay_BusDelay(void)
{
    volatile uint32_t count;

    for (count = 0U; count < 80U; count++)
    {
        __NOP();
    }
}

static bool AppDisplay_RecoverBus(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint32_t pulse;

    ssd1306_UpdateScreenAbort();
    (void)HAL_I2C_DeInit(&hi2c1);
    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Pin = OLED_SCL_Pin | OLED_SDA_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, OLED_SCL_Pin | OLED_SDA_Pin, GPIO_PIN_SET);
    AppDisplay_BusDelay();

    for (pulse = 0U; pulse < 9U; pulse++)
    {
        if (HAL_GPIO_ReadPin(OLED_SDA_GPIO_Port, OLED_SDA_Pin) == GPIO_PIN_SET)
        {
            break;
        }
        HAL_GPIO_WritePin(OLED_SCL_GPIO_Port, OLED_SCL_Pin, GPIO_PIN_RESET);
        AppDisplay_BusDelay();
        HAL_GPIO_WritePin(OLED_SCL_GPIO_Port, OLED_SCL_Pin, GPIO_PIN_SET);
        AppDisplay_BusDelay();
    }

    /* Generate a STOP condition: SDA low -> SCL high -> SDA high. */
    HAL_GPIO_WritePin(OLED_SDA_GPIO_Port, OLED_SDA_Pin, GPIO_PIN_RESET);
    AppDisplay_BusDelay();
    HAL_GPIO_WritePin(OLED_SCL_GPIO_Port, OLED_SCL_Pin, GPIO_PIN_SET);
    AppDisplay_BusDelay();
    HAL_GPIO_WritePin(OLED_SDA_GPIO_Port, OLED_SDA_Pin, GPIO_PIN_SET);
    AppDisplay_BusDelay();

    HAL_GPIO_DeInit(GPIOB, OLED_SCL_Pin | OLED_SDA_Pin);
    return (HAL_I2C_Init(&hi2c1) == HAL_OK);
}

static bool AppDisplay_ProbeAndInit(void)
{
    return (HAL_I2C_IsDeviceReady(&hi2c1,
                                  APP_DISPLAY_I2C_ADDRESS,
                                  2U,
                                  10U) == HAL_OK) &&
           (ssd1306_Init() == SSD1306_OK);
}

static void AppDisplay_SetOffline(uint32_t now_ms)
{
    ssd1306_UpdateScreenAbort();
    app_display_transfer_active = false;
    app_display_ready = false;
    app_display_retry_ms = now_ms + APP_DISPLAY_RETRY_MS;
    AppAlarm_SetCondition(APP_ALARM_OLED, true);
}

void AppDisplay_Init(uint32_t now_ms)
{
    app_display_ready = AppDisplay_ProbeAndInit();
    app_display_transfer_active = false;
    app_display_failure_count = 0U;
    app_display_next_ms = now_ms;
    app_display_retry_ms = now_ms + APP_DISPLAY_RETRY_MS;
    app_display_refresh_requested = true;
    app_display_transfer_started_ms = now_ms;
    app_display_last_success_ms = now_ms;
}

void AppDisplay_Process(uint32_t now_ms)
{
    SSD1306_UpdateStatus_t update_status;

    if (!app_display_ready)
    {
        if ((int32_t)(now_ms - app_display_retry_ms) >= 0)
        {
            app_display_retry_ms = now_ms + APP_DISPLAY_RETRY_MS;
            if (AppDisplay_RecoverBus() && AppDisplay_ProbeAndInit())
            {
                app_display_ready = true;
                app_display_failure_count = 0U;
                app_display_next_ms = now_ms;
                app_display_last_success_ms = now_ms;
                app_display_refresh_requested = true;
                AppAlarm_SetCondition(APP_ALARM_OLED, false);
            }
        }
        return;
    }

    if (app_display_transfer_active)
    {
        if ((uint32_t)(now_ms - app_display_transfer_started_ms) >=
            APP_DISPLAY_FRAME_TIMEOUT_MS)
        {
            AppDisplay_SetOffline(now_ms);
            return;
        }
        update_status = ssd1306_UpdateScreenStep();
        if (update_status == SSD1306_UPDATE_COMPLETE)
        {
            app_display_transfer_active = false;
            app_display_failure_count = 0U;
            app_display_last_success_ms = now_ms;
            AppAlarm_SetCondition(APP_ALARM_OLED, false);
        }
        else if (update_status == SSD1306_UPDATE_ERROR)
        {
            app_display_transfer_active = false;
            app_display_failure_count++;
            if (app_display_failure_count >= APP_DISPLAY_MAX_FAILURES)
            {
                AppDisplay_SetOffline(now_ms);
            }
        }
        return;
    }

    if ((uint32_t)(now_ms - app_display_last_success_ms) >=
        APP_DISPLAY_SUCCESS_WATCHDOG_MS)
    {
        AppDisplay_SetOffline(now_ms);
        return;
    }

    if (!app_display_refresh_requested &&
        ((int32_t)(now_ms - app_display_next_ms) < 0))
    {
        return;
    }
    app_display_next_ms = now_ms + APP_DISPLAY_PERIOD_MS;
    app_display_refresh_requested = false;
    ssd1306_Fill(Black);
    if (AppState_GetPage() == APP_PAGE_CONFIG)
    {
        AppDisplay_DrawConfig();
    }
    else
    {
        AppDisplay_DrawMain();
    }
    ssd1306_UpdateScreenBegin();
    app_display_transfer_active = true;
    app_display_transfer_started_ms = now_ms;
}

void AppDisplay_RequestRefresh(void)
{
    app_display_refresh_requested = true;
}

bool AppDisplay_IsReady(void)
{
    return app_display_ready;
}
