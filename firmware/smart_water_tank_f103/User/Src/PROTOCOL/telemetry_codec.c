#include "PROTOCOL/telemetry_codec.h"

#include "APP/app_alarm.h"
#include "APP/app_config.h"
#include "APP/app_state.h"
#include "BSP/relay.h"
#include "NETWORK/network_config.h"
#include "NETWORK/network_manager.h"
#include "SERVICE/sensor_service.h"

#include <stdio.h>

static const char *TelemetryCodec_ModeText(void)
{
    return (AppState_GetMode() == APP_MODE_AUTO) ? "AUTO" : "MANUAL";
}

static const char *TelemetryCodec_StateText(void)
{
    switch (AppState_GetRunState())
    {
        case APP_RUN_BOOT_WAIT_VALID: return "BOOT_WAIT";
        case APP_RUN_IDLE:            return "IDLE";
        case APP_RUN_RUNNING:         return "RUNNING";
        case APP_RUN_AUTO_PAUSED:     return "AUTO_PAUSED";
        case APP_RUN_FAULT_LOCKED:    return "FAULT_LOCKED";
        default:                      return "UNKNOWN";
    }
}

static bool TelemetryCodec_ResultOk(int result, size_t buffer_size)
{
    return (result > 0) && ((size_t)result < buffer_size);
}

bool TelemetryCodec_EncodeStatusPart(char *buffer,
                                     size_t buffer_size,
                                     uint8_t part)
{
    const SensorServiceData_t *sensor = SensorService_GetData();
    int result;

    if ((buffer == 0) || (buffer_size == 0U))
    {
        return false;
    }
    switch (part)
    {
        case 0U:
            result = snprintf(buffer, buffer_size,
                "{\"v\":1,\"type\":\"status\",\"part\":\"control\","
                "\"device_id\":\"%s\",\"mode\":\"%s\","
                "\"state\":\"%s\",\"pump\":%u,\"alarm_flags\":%lu,"
                "\"alarm\":\"%s\",\"muted\":%u}",
                MQTT_CLIENT_ID, TelemetryCodec_ModeText(),
                TelemetryCodec_StateText(),
                Relay_IsOn() ? 1U : 0U,
                (unsigned long)AppAlarm_GetFlags(),
                AppAlarm_GetPrimaryText(), AppAlarm_IsMuted() ? 1U : 0U);
            break;
        case 1U:
            result = snprintf(buffer, buffer_size,
                "{\"v\":1,\"type\":\"status\",\"part\":\"level\","
                "\"device_id\":\"%s\",\"level\":%u,"
                "\"distance_mm\":%lu,\"ultrasonic_valid\":%u}",
                MQTT_CLIENT_ID,
                (unsigned int)sensor->level_percent,
                (unsigned long)sensor->distance_mm,
                sensor->ultrasonic_valid ? 1U : 0U);
            break;
        case 2U:
            result = snprintf(buffer, buffer_size,
                "{\"v\":1,\"type\":\"status\",\"part\":\"pressure_raw\","
                "\"device_id\":\"%s\","
                "\"pressure_raw\":%ld,\"pressure_delta\":%ld,"
                "\"pressure_valid\":%u}",
                MQTT_CLIENT_ID,
                (long)sensor->pressure_raw,
                (long)sensor->pressure_delta_raw,
                sensor->pressure_valid ? 1U : 0U);
            break;
        case 3U:
            result = snprintf(buffer, buffer_size,
                "{\"v\":1,\"type\":\"status\",\"part\":\"network\","
                "\"device_id\":\"%s\",\"uptime_ms\":%lu,\"csq\":%d,"
                "\"network\":\"ONLINE\"}",
                MQTT_CLIENT_ID, (unsigned long)HAL_GetTick(),
                NetworkManager_GetSignalQuality());
            break;
        default:
            return false;
    }
    return TelemetryCodec_ResultOk(result, buffer_size);
}

bool TelemetryCodec_EncodeConfigPart(char *buffer,
                                     size_t buffer_size,
                                     uint8_t part)
{
    const AppConfigData_t *config = AppConfig_GetActive();
    int result;

    if ((buffer == 0) || (buffer_size == 0U))
    {
        return false;
    }
    if (part == 0U)
    {
        result = snprintf(buffer, buffer_size,
            "{\"v\":1,\"type\":\"config\",\"part\":\"level\","
            "\"device_id\":\"%s\",\"low_level\":%u,"
            "\"high_level\":%u}",
            MQTT_CLIENT_ID,
            (unsigned int)config->low_level_percent,
            (unsigned int)config->high_level_percent);
    }
    else if (part == 1U)
    {
        result = snprintf(buffer, buffer_size,
            "{\"v\":1,\"type\":\"config\",\"part\":\"safety\","
            "\"device_id\":\"%s\","
            "\"pressure_trip\":%ld,\"pressure_recover\":%ld,"
            "\"max_run_min\":%u}",
            MQTT_CLIENT_ID,
            (long)config->pressure_trip_delta_raw,
            (long)config->pressure_recover_delta_raw,
            (unsigned int)config->max_run_minutes);
    }
    else
    {
        return false;
    }
    return TelemetryCodec_ResultOk(result, buffer_size);
}

bool TelemetryCodec_EncodeAlarm(char *buffer, size_t buffer_size)
{
    int result;

    if ((buffer == 0) || (buffer_size == 0U))
    {
        return false;
    }
    result = snprintf(buffer, buffer_size,
        "{\"v\":1,\"type\":\"alarm\",\"device_id\":\"%s\",\"uptime_ms\":%lu,"
        "\"alarm_flags\":%lu,\"alarm\":\"%s\",\"muted\":%u,"
        "\"pump\":%u}",
        MQTT_CLIENT_ID, (unsigned long)HAL_GetTick(),
        (unsigned long)AppAlarm_GetFlags(), AppAlarm_GetPrimaryText(),
        AppAlarm_IsMuted() ? 1U : 0U, Relay_IsOn() ? 1U : 0U);
    return TelemetryCodec_ResultOk(result, buffer_size);
}

bool TelemetryCodec_EncodeAck(char *buffer,
                              size_t buffer_size,
                              uint32_t message_id,
                              const char *result_text,
                              const char *reason)
{
    int result;

    if ((buffer == 0) || (buffer_size == 0U) ||
        (result_text == 0) || (reason == 0))
    {
        return false;
    }
    result = snprintf(buffer, buffer_size,
        "{\"v\":1,\"type\":\"ack\",\"device_id\":\"%s\",\"id\":%lu,"
        "\"result\":\"%s\",\"reason\":\"%s\"}",
        MQTT_CLIENT_ID, (unsigned long)message_id,
        result_text, reason);
    return TelemetryCodec_ResultOk(result, buffer_size);
}
