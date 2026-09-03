#include "NETWORK/network_manager.h"

#include "NETWORK/at_client.h"
#include "NETWORK/ml307c_at.h"
#include "NETWORK/ml307c_mqtt.h"
#include "NETWORK/network_config.h"
#include "APP/app_alarm.h"
#include "APP/app_command.h"
#include "APP/app_config.h"
#include "APP/app_control.h"
#include "APP/app_state.h"
#include "BSP/relay.h"
#include "PROTOCOL/command_codec.h"
#include "PROTOCOL/mqtt_topics.h"
#include "PROTOCOL/telemetry_codec.h"

#include <stdio.h>
#include <string.h>

#define NETWORK_BOOT_WAIT_MS             5000U
#define NETWORK_SHORT_TIMEOUT_MS         4000U
#define NETWORK_CONNECT_TIMEOUT_MS      30000U
#define NETWORK_MQTT_OP_TIMEOUT_MS      12000U
#define NETWORK_RETRY_MS                 5000U
#define NETWORK_LOCAL_RETRY_MS           2000U
#define NETWORK_SIM_WAIT_MS             30000U
#define NETWORK_REGISTRATION_WAIT_MS   120000U
#define NETWORK_ATTACH_WAIT_MS          60000U
#define NETWORK_PUBLISH_PERIOD_MS       10000U
#define NETWORK_PUBLISH_GAP_MS           1000U
#define NETWORK_RESPONSE_SIZE            1024U
#define NETWORK_COMMAND_SIZE              768U
#define NETWORK_ERROR_SIZE                 24U
#define NETWORK_TOPIC_SIZE                  64U
#define NETWORK_PAYLOAD_SIZE               512U
#define NETWORK_INBOUND_LINE_SIZE          512U
#define NETWORK_ACK_QUEUE_SIZE               8U
#define NETWORK_ACK_TEXT_SIZE               20U
#define NETWORK_AT_ATTEMPTS                  3U
#define NETWORK_PUBLISH_ATTEMPTS             2U
#define ML307C_CONNECT_ID                    0

typedef enum
{
    NET_STEP_BOOT = 0,
    NET_STEP_CLEAN,
    NET_STEP_AT,
    NET_STEP_ECHO_OFF,
    NET_STEP_SIM,
    NET_STEP_CARD_ID,
    NET_STEP_SIGNAL,
    NET_STEP_REGISTRATION,
    NET_STEP_ATTACH,
    NET_STEP_MQTT_QUERY,
    NET_STEP_CFG_VERSION,
    NET_STEP_CFG_KEEPALIVE,
    NET_STEP_CFG_CLEAN,
    NET_STEP_CFG_PINGRESP,
    NET_STEP_CONNECT,
    NET_STEP_SUB_COMMAND,
    NET_STEP_SUB_CONFIG,
    NET_STEP_PUBLISH,
    NET_STEP_ONLINE,
    NET_STEP_RETRY
} NetworkStep_t;

typedef enum
{
    NETWORK_PUBLISH_NONE = 0,
    NETWORK_PUBLISH_ACK,
    NETWORK_PUBLISH_ALARM,
    NETWORK_PUBLISH_CONFIG,
    NETWORK_PUBLISH_STATUS
} NetworkPublishKind_t;

typedef struct
{
    uint32_t id;
    char result[8];
    char reason[NETWORK_ACK_TEXT_SIZE];
} NetworkAck_t;

static ATClient_t network_at;
static Network_State_t network_state;
static NetworkStep_t network_step;
static Network_Stage_t network_failed_stage;
static char network_response[NETWORK_RESPONSE_SIZE];
static char network_command[NETWORK_COMMAND_SIZE];
static char network_last_error[NETWORK_ERROR_SIZE];
static char network_publish_topic[NETWORK_TOPIC_SIZE];
static char network_publish_payload[NETWORK_PAYLOAD_SIZE];
/* C/S + uint8_t decimal value + terminator. The normal values are C1..C2
 * and S1..S4; six bytes also makes the defensive uint8_t range safe. */
static char network_publish_label[6];
static char network_publish_trace[12];
static char network_inbound_line[NETWORK_INBOUND_LINE_SIZE];
static char network_inbound_topic[NETWORK_TOPIC_SIZE];
static char network_inbound_payload[NETWORK_PAYLOAD_SIZE];
static NetworkAck_t network_ack_queue[NETWORK_ACK_QUEUE_SIZE];
static uint16_t network_response_length;
static uint16_t network_urc_scan_offset;
static uint32_t network_step_started_ms;
static uint32_t network_stage_deadline_ms;
static uint32_t network_not_before_ms;
static uint32_t network_next_action_ms;
static uint32_t network_seen_uart_errors;
static uint32_t network_seen_overflows;
static uint16_t network_attempt_count;
static uint8_t network_ack_head;
static uint8_t network_ack_tail;
static uint8_t network_ack_count;
static bool network_enabled;
static bool network_command_pending;
static bool network_status_pending;
static bool network_config_pending;
static bool network_alarm_pending;
static NetworkPublishKind_t network_publish_kind;
static uint8_t network_status_part;
static uint8_t network_config_part;
static uint32_t network_last_alarm_flags;
static AppMode_t network_last_mode;
static AppRunState_t network_last_run_state;
static bool network_last_pump;
static bool network_last_muted;
static AppConfigData_t network_last_config;
static bool network_last_message_valid;
static uint32_t network_last_message_id;
static char network_last_ack_result[8];
static char network_last_ack_reason[NETWORK_ACK_TEXT_SIZE];
static int network_csq;
static int network_reg_status;
static int network_conn_state;
static int network_cme_error;
static int network_publish_message_id;

static bool Network_TimeReached(uint32_t now_ms, uint32_t target_ms)
{
    return ((int32_t)(now_ms - target_ms) >= 0);
}

static Network_Stage_t Network_StageForStep(NetworkStep_t step)
{
    switch (step)
    {
        case NET_STEP_BOOT:             return NETWORK_STAGE_BOOT;
        case NET_STEP_CLEAN:
        case NET_STEP_AT:
        case NET_STEP_ECHO_OFF:         return NETWORK_STAGE_AT;
        case NET_STEP_SIM:
        case NET_STEP_CARD_ID:          return NETWORK_STAGE_SIM;
        case NET_STEP_SIGNAL:           return NETWORK_STAGE_SIGNAL;
        case NET_STEP_REGISTRATION:     return NETWORK_STAGE_REGISTER;
        case NET_STEP_ATTACH:           return NETWORK_STAGE_ATTACH;
        case NET_STEP_MQTT_QUERY:
        case NET_STEP_CFG_VERSION:
        case NET_STEP_CFG_KEEPALIVE:
        case NET_STEP_CFG_CLEAN:
        case NET_STEP_CFG_PINGRESP:     return NETWORK_STAGE_MQTT_CONFIG;
        case NET_STEP_CONNECT:          return NETWORK_STAGE_MQTT_CONNECT;
        case NET_STEP_SUB_COMMAND:
        case NET_STEP_SUB_CONFIG:       return NETWORK_STAGE_SUBSCRIBE;
        case NET_STEP_PUBLISH:          return NETWORK_STAGE_PUBLISH;
        case NET_STEP_ONLINE:           return NETWORK_STAGE_ONLINE;
        case NET_STEP_RETRY:
        default:                        return NETWORK_STAGE_OFF;
    }
}

static void Network_SetError(const char *text)
{
    if (text == network_last_error)
    {
        return;
    }
    (void)strncpy(network_last_error,
                  (text != 0) ? text : "UNKNOWN",
                  NETWORK_ERROR_SIZE - 1U);
    network_last_error[NETWORK_ERROR_SIZE - 1U] = '\0';
}

static void Network_ClearResponse(void)
{
    /* The ring buffer was already drained at the start of this update.
     * Do not flush it here: an MQTT downlink URC can arrive between response
     * processing and the next AT command and must not be silently dropped. */
    network_response_length = 0U;
    network_urc_scan_offset = 0U;
    network_response[0] = '\0';
}

static void Network_ReadResponse(void)
{
    uint8_t byte;

    while (ATClient_ReadByte(&network_at, &byte))
    {
        if (network_response_length < (NETWORK_RESPONSE_SIZE - 1U))
        {
            network_response[network_response_length++] = (char)byte;
            network_response[network_response_length] = '\0';
        }
    }
}

static void Network_CopyText(char *destination,
                             size_t destination_size,
                             const char *source)
{
    if ((destination == 0) || (destination_size == 0U))
    {
        return;
    }
    if (destination == source)
    {
        return;
    }
    (void)strncpy(destination, (source != 0) ? source : "",
                  destination_size - 1U);
    destination[destination_size - 1U] = '\0';
}

static void Network_SetPublishTrace(const char *label, const char *result)
{
    (void)snprintf(network_publish_trace, sizeof(network_publish_trace),
                   "%s:%s", (label != 0) ? label : "--",
                   (result != 0) ? result : "?");
    network_publish_trace[sizeof(network_publish_trace) - 1U] = '\0';
}

static void Network_CapturePublishMessageId(void)
{
    int message_id;

    if ((network_step != NET_STEP_PUBLISH) ||
        !network_command_pending ||
        (network_publish_message_id >= 0))
    {
        return;
    }
    message_id = ML307C_MQTT_ParsePublishMessageId(network_response,
                                                    ML307C_CONNECT_ID);
    if (message_id >= 0)
    {
        network_publish_message_id = message_id;
        Network_SetPublishTrace(network_publish_label, "MID");
    }
}

static void Network_QueueAck(uint32_t id,
                             const char *result,
                             const char *reason)
{
    NetworkAck_t *ack;

    if (network_ack_count >= NETWORK_ACK_QUEUE_SIZE)
    {
        network_ack_head = (uint8_t)((network_ack_head + 1U) %
                                     NETWORK_ACK_QUEUE_SIZE);
        network_ack_count--;
    }
    ack = &network_ack_queue[network_ack_tail];
    ack->id = id;
    Network_CopyText(ack->result, sizeof(ack->result), result);
    Network_CopyText(ack->reason, sizeof(ack->reason), reason);
    network_ack_tail = (uint8_t)((network_ack_tail + 1U) %
                                 NETWORK_ACK_QUEUE_SIZE);
    network_ack_count++;

    network_last_message_valid = true;
    network_last_message_id = id;
    Network_CopyText(network_last_ack_result,
                     sizeof(network_last_ack_result), result);
    Network_CopyText(network_last_ack_reason,
                     sizeof(network_last_ack_reason), reason);
}

static bool Network_PostRemoteCommand(AppCommandType_t type,
                                      uint32_t now_ms)
{
    return AppCommand_Post(type, APP_COMMAND_SOURCE_MQTT, now_ms);
}

static void Network_HandleCommandPayload(const char *payload,
                                         uint32_t now_ms)
{
    uint32_t id = 0U;
    RemoteCommandType_t command = REMOTE_COMMAND_INVALID;
    bool accepted = false;
    const char *result = "FAIL";
    const char *reason = "INVALID_FORMAT";

    if (!CommandCodec_ParseCommand(payload, &id, &command))
    {
        Network_QueueAck(id, result, reason);
        return;
    }
    if (network_last_message_valid && (id == network_last_message_id))
    {
        Network_QueueAck(id, network_last_ack_result,
                         network_last_ack_reason);
        return;
    }
    if (AppConfig_IsEditing() ||
        (AppState_GetPage() == APP_PAGE_CONFIG))
    {
        Network_QueueAck(id, "FAIL", "BUSY");
        return;
    }

    switch (command)
    {
        case REMOTE_COMMAND_START:
            if (!AppControl_CanStart())
            {
                reason = AppControl_GetStartDeniedReason();
            }
            else
            {
                accepted = Network_PostRemoteCommand(APP_COMMAND_START,
                                                     now_ms);
                reason = accepted ? "ACCEPTED" : "QUEUE_FULL";
            }
            break;

        case REMOTE_COMMAND_STOP:
            if (AppState_GetMode() != APP_MODE_MANUAL)
            {
                reason = "MANUAL_ONLY";
            }
            else
            {
                accepted = Network_PostRemoteCommand(APP_COMMAND_STOP,
                                                     now_ms);
                reason = accepted ? "ACCEPTED" : "QUEUE_FULL";
            }
            break;

        case REMOTE_COMMAND_AUTO:
            if (AppState_GetMode() == APP_MODE_AUTO)
            {
                accepted = true;
                reason = "ALREADY_AUTO";
            }
            else
            {
                accepted = Network_PostRemoteCommand(APP_COMMAND_MODE_TOGGLE,
                                                     now_ms);
                reason = accepted ? "ACCEPTED" : "QUEUE_FULL";
            }
            break;

        case REMOTE_COMMAND_MANUAL:
            if (AppState_GetMode() == APP_MODE_MANUAL)
            {
                accepted = true;
                reason = "ALREADY_MANUAL";
            }
            else
            {
                accepted = Network_PostRemoteCommand(APP_COMMAND_MODE_TOGGLE,
                                                     now_ms);
                reason = accepted ? "ACCEPTED" : "QUEUE_FULL";
            }
            break;

        case REMOTE_COMMAND_MUTE:
            if (!AppAlarm_HasAny())
            {
                reason = "NO_ALARM";
            }
            else if (AppAlarm_IsMuted())
            {
                accepted = true;
                reason = "ALREADY_MUTED";
            }
            else
            {
                accepted = Network_PostRemoteCommand(APP_COMMAND_MUTE_TOGGLE,
                                                     now_ms);
                reason = accepted ? "ACCEPTED" : "QUEUE_FULL";
            }
            break;

        case REMOTE_COMMAND_STATUS:
            accepted = true;
            reason = "STATUS_QUEUED";
            break;

        default:
            reason = "UNKNOWN_COMMAND";
            break;
    }

    result = accepted ? "OK" : "FAIL";
    Network_QueueAck(id, result, reason);
    network_status_pending = true;
    if ((command == REMOTE_COMMAND_MUTE) && accepted)
    {
        network_alarm_pending = true;
    }
}

static void Network_HandleConfigPayload(const char *payload)
{
    uint32_t id = 0U;
    AppConfigData_t config;
    AppConfigApplyResult_t apply_result;

    if (!CommandCodec_ParseConfig(payload, &id, &config))
    {
        Network_QueueAck(id, "FAIL", "INVALID_CONFIG");
        return;
    }
    if (network_last_message_valid && (id == network_last_message_id))
    {
        Network_QueueAck(id, network_last_ack_result,
                         network_last_ack_reason);
        return;
    }
    apply_result = AppConfig_ApplyRemote(&config);
    switch (apply_result)
    {
        case APP_CONFIG_APPLY_OK:
            AppAlarm_SetCondition(APP_ALARM_CONFIG, false);
            Network_QueueAck(id, "OK", "SAVED");
            network_config_pending = true;
            network_status_pending = true;
            break;
        case APP_CONFIG_APPLY_BUSY:
            Network_QueueAck(id, "FAIL", "BUSY");
            break;
        case APP_CONFIG_APPLY_SAVE_FAILED:
            AppAlarm_SetCondition(APP_ALARM_CONFIG, true);
            Network_QueueAck(id, "FAIL", "SAVE_FAILED");
            network_alarm_pending = true;
            break;
        case APP_CONFIG_APPLY_INVALID:
        default:
            Network_QueueAck(id, "FAIL", "INVALID_CONFIG");
            break;
    }
}

static void Network_HandleInboundPublish(const char *topic,
                                         const char *payload,
                                         uint32_t now_ms)
{
    if (strcmp(topic, MQTT_DOWNLINK_COMMAND_TOPIC) == 0)
    {
        Network_HandleCommandPayload(payload, now_ms);
    }
    else if (strcmp(topic, MQTT_DOWNLINK_CONFIG_TOPIC) == 0)
    {
        Network_HandleConfigPayload(payload);
    }
}

static void Network_ProcessInboundUrcs(uint32_t now_ms)
{
    const char *line_start;
    const char *line_end;
    size_t line_length;

    while (network_urc_scan_offset < network_response_length)
    {
        line_start = &network_response[network_urc_scan_offset];
        line_end = strstr(line_start, "\r\n");
        if (line_end == 0)
        {
            return;
        }
        line_length = (size_t)(line_end - line_start);
        network_urc_scan_offset =
            (uint16_t)((line_end - network_response) + 2U);

        if ((line_length == 0U) ||
            (line_length >= sizeof(network_inbound_line)))
        {
            continue;
        }
        (void)memcpy(network_inbound_line, line_start, line_length);
        network_inbound_line[line_length] = '\0';
        if (ML307C_MQTT_ExtractPublish(network_inbound_line,
                                       ML307C_CONNECT_ID,
                                       network_inbound_topic,
                                       sizeof(network_inbound_topic),
                                       network_inbound_payload,
                                       sizeof(network_inbound_payload)))
        {
            Network_HandleInboundPublish(network_inbound_topic,
                                         network_inbound_payload,
                                         now_ms);
        }
    }
}

static void Network_DetectLocalChanges(void)
{
    uint32_t alarm_flags = AppAlarm_GetFlags();
    AppMode_t mode = AppState_GetMode();
    AppRunState_t run_state = AppState_GetRunState();
    bool pump = Relay_IsOn();
    bool muted = AppAlarm_IsMuted();
    const AppConfigData_t *config = AppConfig_GetActive();

    if (alarm_flags != network_last_alarm_flags)
    {
        network_last_alarm_flags = alarm_flags;
        network_alarm_pending = true;
        network_status_pending = true;
    }
    if (muted != network_last_muted)
    {
        network_last_muted = muted;
        network_alarm_pending = true;
        network_status_pending = true;
    }
    if ((mode != network_last_mode) ||
        (run_state != network_last_run_state) ||
        (pump != network_last_pump))
    {
        network_last_mode = mode;
        network_last_run_state = run_state;
        network_last_pump = pump;
        network_status_pending = true;
    }
    if (memcmp(config, &network_last_config, sizeof(*config)) != 0)
    {
        network_last_config = *config;
        network_config_pending = true;
        network_status_pending = true;
    }
}

static bool Network_PreparePublish(void)
{
    const NetworkAck_t *ack;
    const char *topic = 0;
    bool encoded = false;

    network_publish_kind = NETWORK_PUBLISH_NONE;
    if (network_ack_count > 0U)
    {
        ack = &network_ack_queue[network_ack_head];
        topic = MQTT_UPLINK_ACK_TOPIC;
        encoded = TelemetryCodec_EncodeAck(network_publish_payload,
                                            sizeof(network_publish_payload),
                                            ack->id, ack->result,
                                            ack->reason);
        network_publish_kind = NETWORK_PUBLISH_ACK;
    }
    else if (network_alarm_pending)
    {
        topic = MQTT_UPLINK_ALARM_TOPIC;
        encoded = TelemetryCodec_EncodeAlarm(network_publish_payload,
                                              sizeof(network_publish_payload));
        network_publish_kind = NETWORK_PUBLISH_ALARM;
    }
    else if (network_config_pending)
    {
        topic = MQTT_UPLINK_STATUS_TOPIC;
        encoded = TelemetryCodec_EncodeConfigPart(
                                               network_publish_payload,
                                               sizeof(network_publish_payload),
                                               network_config_part);
        network_publish_kind = NETWORK_PUBLISH_CONFIG;
    }
    else if (network_status_pending)
    {
        topic = MQTT_UPLINK_STATUS_TOPIC;
        encoded = TelemetryCodec_EncodeStatusPart(
                                               network_publish_payload,
                                               sizeof(network_publish_payload),
                                               network_status_part);
        network_publish_kind = NETWORK_PUBLISH_STATUS;
    }

    if (!encoded || (topic == 0))
    {
        network_publish_kind = NETWORK_PUBLISH_NONE;
        return false;
    }
    Network_CopyText(network_publish_topic,
                     sizeof(network_publish_topic), topic);
    switch (network_publish_kind)
    {
        case NETWORK_PUBLISH_ACK:    Network_CopyText(network_publish_label, sizeof(network_publish_label), "AK"); break;
        case NETWORK_PUBLISH_ALARM:  Network_CopyText(network_publish_label, sizeof(network_publish_label), "AL"); break;
        case NETWORK_PUBLISH_CONFIG: (void)snprintf(network_publish_label, sizeof(network_publish_label), "C%u", (unsigned int)network_config_part + 1U); break;
        case NETWORK_PUBLISH_STATUS: (void)snprintf(network_publish_label, sizeof(network_publish_label), "S%u", (unsigned int)network_status_part + 1U); break;
        default:                      Network_CopyText(network_publish_label, sizeof(network_publish_label), "--"); break;
    }
    Network_SetPublishTrace(network_publish_label, "WAIT");
    return true;
}

static void Network_CompletePublish(void)
{
    Network_SetPublishTrace(network_publish_label, "ACK");
    switch (network_publish_kind)
    {
        case NETWORK_PUBLISH_ACK:
            if (network_ack_count > 0U)
            {
                network_ack_head = (uint8_t)((network_ack_head + 1U) %
                                             NETWORK_ACK_QUEUE_SIZE);
                network_ack_count--;
            }
            break;
        case NETWORK_PUBLISH_ALARM:
            network_alarm_pending = false;
            break;
        case NETWORK_PUBLISH_CONFIG:
            network_config_part++;
            if (network_config_part >= TELEMETRY_CONFIG_PART_COUNT)
            {
                network_config_part = 0U;
                network_config_pending = false;
            }
            break;
        case NETWORK_PUBLISH_STATUS:
            network_status_part++;
            if (network_status_part >= TELEMETRY_STATUS_PART_COUNT)
            {
                network_status_part = 0U;
                network_status_pending = false;
            }
            break;
        default:
            break;
    }
    network_publish_kind = NETWORK_PUBLISH_NONE;
}

static bool Network_ProtocolSelfTest(void)
{
    static const char sample_urc[] =
        "+MQTTURC: \"publish\",0,7,\"test/cmd\",23,23,"
        "{\"id\":1,\"cmd\":\"STATUS\"}";
    char topic[16];
    char payload[40];
    uint32_t id = 0U;
    RemoteCommandType_t command = REMOTE_COMMAND_INVALID;

    return ML307C_MQTT_ExtractPublish(sample_urc,
                                      ML307C_CONNECT_ID,
                                      topic, sizeof(topic),
                                      payload, sizeof(payload)) &&
           (strcmp(topic, "test/cmd") == 0) &&
           CommandCodec_ParseCommand(payload, &id, &command) &&
           (id == 1U) && (command == REMOTE_COMMAND_STATUS);
}

static bool Network_ResponseFinished(void)
{
    if (ML307C_AT_HasError(network_response))
    {
        return true;
    }
    if (network_step == NET_STEP_CONNECT)
    {
        return (ML307C_MQTT_ParseConnectionState(network_response,
                                                  ML307C_CONNECT_ID) >= 0);
    }
    if ((network_step == NET_STEP_SUB_COMMAND) ||
        (network_step == NET_STEP_SUB_CONFIG))
    {
        return ML307C_MQTT_HasSubAck(network_response,
                                     ML307C_CONNECT_ID) ||
               ML307C_MQTT_HasTimeout(network_response);
    }
    if (network_step == NET_STEP_PUBLISH)
    {
        return (network_publish_message_id >= 0) &&
               (ML307C_MQTT_HasPubAckForMessage(network_response,
                                                 ML307C_CONNECT_ID,
                                                 network_publish_message_id) ||
                ML307C_MQTT_HasTimeout(network_response));
    }
    return ML307C_AT_IsOk(network_response);
}

static void Network_EnterStep(NetworkStep_t step, uint32_t now_ms)
{
    network_step = step;
    network_attempt_count = 0U;
    network_command_pending = false;
    network_not_before_ms = now_ms;
    network_stage_deadline_ms = now_ms;

    if (step == NET_STEP_PUBLISH)
    {
        /* Keep short back-to-back frames from overrunning the modem's AT
         * parser immediately after the previous PUBACK. */
        network_not_before_ms = now_ms + NETWORK_PUBLISH_GAP_MS;
        network_publish_message_id = -1;
    }

    if (step == NET_STEP_SIM)
    {
        network_stage_deadline_ms = now_ms + NETWORK_SIM_WAIT_MS;
    }
    else if (step == NET_STEP_REGISTRATION)
    {
        network_stage_deadline_ms = now_ms + NETWORK_REGISTRATION_WAIT_MS;
    }
    else if (step == NET_STEP_ATTACH)
    {
        network_stage_deadline_ms = now_ms + NETWORK_ATTACH_WAIT_MS;
    }
}

static void Network_GoToRetry(uint32_t now_ms,
                              Network_Stage_t failed_stage,
                              const char *reason)
{
    network_failed_stage = failed_stage;
    if (network_cme_error >= 0)
    {
        (void)snprintf(network_last_error, sizeof(network_last_error),
                       "CME:%d", network_cme_error);
    }
    else
    {
        if (failed_stage == NETWORK_STAGE_PUBLISH)
        {
            (void)snprintf(network_last_error, sizeof(network_last_error),
                           "P:%s %s", network_publish_label,
                           (reason != 0) ? reason : "FAIL");
        }
        else
        {
            Network_SetError(reason);
        }
    }
    network_state = NETWORK_STATE_RETRY_WAIT;
    network_step = NET_STEP_RETRY;
    network_command_pending = false;
    network_next_action_ms = now_ms + NETWORK_RETRY_MS;
    Network_ClearResponse();
}

static void Network_RepeatStep(uint32_t now_ms, uint32_t delay_ms)
{
    network_command_pending = false;
    network_not_before_ms = now_ms + delay_ms;
    Network_ClearResponse();
}

static const char *Network_CommandForStep(void)
{
    switch (network_step)
    {
        case NET_STEP_CLEAN:
            return "AT+MQTTDISC=0\r\n";
        case NET_STEP_AT:
            return "AT\r\n";
        case NET_STEP_ECHO_OFF:
            return "ATE0\r\n";
        case NET_STEP_SIM:
            return "AT+CPIN?\r\n";
        case NET_STEP_CARD_ID:
            return "AT+CCID\r\n";
        case NET_STEP_SIGNAL:
            return "AT+CSQ\r\n";
        case NET_STEP_REGISTRATION:
            return "AT+CEREG?\r\n";
        case NET_STEP_ATTACH:
            return "AT+CGATT?\r\n";
        case NET_STEP_MQTT_QUERY:
            return "AT+MQTTCFG=?\r\n";
        case NET_STEP_CFG_VERSION:
            return "AT+MQTTCFG=\"version\",0,4\r\n";
        case NET_STEP_CFG_KEEPALIVE:
            return "AT+MQTTCFG=\"keepalive\",0,60\r\n";
        case NET_STEP_CFG_CLEAN:
            return "AT+MQTTCFG=\"clean\",0,1\r\n";
        case NET_STEP_CFG_PINGRESP:
            return "AT+MQTTCFG=\"pingresp\",0,1\r\n";
        case NET_STEP_CONNECT:
            return ML307C_MQTT_BuildConnect(network_command,
                                             sizeof(network_command),
                                             ML307C_CONNECT_ID,
                                             MQTT_HOST, MQTT_PORT,
                                             MQTT_CLIENT_ID,
                                             MQTT_USERNAME,
                                             MQTT_PASSWORD) ?
                   network_command : 0;
        case NET_STEP_SUB_COMMAND:
            return ML307C_MQTT_BuildSubscribe(network_command,
                                               sizeof(network_command),
                                               ML307C_CONNECT_ID,
                                               MQTT_TOPIC_COMMAND, 1) ?
                   network_command : 0;
        case NET_STEP_SUB_CONFIG:
            return ML307C_MQTT_BuildSubscribe(network_command,
                                               sizeof(network_command),
                                               ML307C_CONNECT_ID,
                                               MQTT_TOPIC_CONFIG, 1) ?
                   network_command : 0;
        case NET_STEP_PUBLISH:
            return ML307C_MQTT_BuildPublish(
                                             network_command,
                                             sizeof(network_command),
                                             ML307C_CONNECT_ID,
                                             network_publish_topic, 1,
                                             network_publish_payload) ?
                   network_command : 0;
        default:
            return 0;
    }
}

static uint32_t Network_TimeoutMs(void)
{
    if (network_step == NET_STEP_CONNECT)
    {
        return NETWORK_CONNECT_TIMEOUT_MS;
    }
    if ((network_step == NET_STEP_SUB_COMMAND) ||
        (network_step == NET_STEP_SUB_CONFIG) ||
        (network_step == NET_STEP_PUBLISH))
    {
        return NETWORK_MQTT_OP_TIMEOUT_MS;
    }
    return NETWORK_SHORT_TIMEOUT_MS;
}

static bool Network_SendCommand(const char *command, uint32_t now_ms)
{
    if (network_step == NET_STEP_PUBLISH)
    {
        /* A retry receives a new modem message-id; never accept the ACK for
         * the first attempt as the ACK for the retry. */
        network_publish_message_id = -1;
    }
    Network_ClearResponse();
    if (ATClient_Send(&network_at, command) != HAL_OK)
    {
        return false;
    }
    network_attempt_count++;
    network_step_started_ms = now_ms;
    network_command_pending = true;
    return true;
}

static bool Network_Evaluate(void)
{
    network_cme_error = ML307C_AT_ParseCmeError(network_response);

    switch (network_step)
    {
        case NET_STEP_CLEAN:
        case NET_STEP_ECHO_OFF:
        case NET_STEP_CARD_ID:
            return true;
        case NET_STEP_AT:
            return ML307C_AT_IsOk(network_response);
        case NET_STEP_SIM:
            return ML307C_AT_IsSimReady(network_response);
        case NET_STEP_SIGNAL:
            network_csq = ML307C_AT_ParseSignalQuality(network_response);
            /* CSQ is diagnostic. Registration is the authoritative gate. */
            return true;
        case NET_STEP_REGISTRATION:
            network_reg_status =
                ML307C_AT_ParseRegistration(network_response);
            return (network_reg_status == 1) || (network_reg_status == 5);
        case NET_STEP_ATTACH:
            return ML307C_AT_IsAttached(network_response);
        case NET_STEP_MQTT_QUERY:
            return ML307C_MQTT_IsSupported(network_response);
        case NET_STEP_CFG_VERSION:
        case NET_STEP_CFG_KEEPALIVE:
        case NET_STEP_CFG_CLEAN:
        case NET_STEP_CFG_PINGRESP:
            return ML307C_AT_IsOk(network_response);
        case NET_STEP_CONNECT:
            network_conn_state =
                ML307C_MQTT_ParseConnectionState(network_response,
                                                  ML307C_CONNECT_ID);
            return (network_conn_state == 0);
        case NET_STEP_SUB_COMMAND:
        case NET_STEP_SUB_CONFIG:
            return ML307C_MQTT_HasSubAck(network_response,
                                         ML307C_CONNECT_ID);
        case NET_STEP_PUBLISH:
            return (network_publish_message_id >= 0) &&
                   ML307C_MQTT_HasPubAckForMessage(network_response,
                                                     ML307C_CONNECT_ID,
                                                     network_publish_message_id);
        default:
            return false;
    }
}

static void Network_Advance(uint32_t now_ms)
{
    if (network_step == NET_STEP_SUB_CONFIG)
    {
        Network_EnterStep(NET_STEP_ONLINE, now_ms);
        network_state = NETWORK_STATE_ONLINE;
        network_next_action_ms = now_ms + NETWORK_PUBLISH_PERIOD_MS;
        network_status_pending = true;
        network_config_pending = true;
        network_status_part = 0U;
        network_config_part = 0U;
        network_alarm_pending = AppAlarm_HasAny();
        network_failed_stage = NETWORK_STAGE_OFF;
        Network_SetError("NONE");
        Network_ClearResponse();
        return;
    }
    if (network_step == NET_STEP_PUBLISH)
    {
        Network_CompletePublish();
        Network_EnterStep(NET_STEP_ONLINE, now_ms);
        network_state = NETWORK_STATE_ONLINE;
        network_failed_stage = NETWORK_STAGE_OFF;
        Network_SetError("NONE");
        Network_ClearResponse();
        return;
    }

    Network_EnterStep((NetworkStep_t)((uint32_t)network_step + 1U), now_ms);
}

static void Network_HandleStepResult(uint32_t now_ms,
                                     bool response_ok,
                                     bool timed_out)
{
    Network_Stage_t stage = Network_StageForStep(network_step);

    if (response_ok)
    {
        Network_Advance(now_ms);
        return;
    }

    if ((network_step == NET_STEP_AT) &&
        (network_attempt_count < NETWORK_AT_ATTEMPTS))
    {
        Network_RepeatStep(now_ms, 1000U);
        return;
    }
    if ((network_step == NET_STEP_PUBLISH) &&
        (network_attempt_count < NETWORK_PUBLISH_ATTEMPTS))
    {
        Network_RepeatStep(now_ms, 500U);
        return;
    }
    if ((network_step == NET_STEP_SIM) &&
        !Network_TimeReached(now_ms, network_stage_deadline_ms))
    {
        Network_RepeatStep(now_ms, NETWORK_LOCAL_RETRY_MS);
        return;
    }
    if ((network_step == NET_STEP_REGISTRATION) &&
        !Network_TimeReached(now_ms, network_stage_deadline_ms))
    {
        if (network_reg_status >= 0)
        {
            (void)snprintf(network_last_error, sizeof(network_last_error),
                           "REG:%d", network_reg_status);
        }
        Network_RepeatStep(now_ms, NETWORK_LOCAL_RETRY_MS);
        return;
    }
    if ((network_step == NET_STEP_ATTACH) &&
        !Network_TimeReached(now_ms, network_stage_deadline_ms))
    {
        Network_RepeatStep(now_ms, NETWORK_LOCAL_RETRY_MS);
        return;
    }

    if (timed_out)
    {
        Network_GoToRetry(now_ms, stage, "TIMEOUT");
    }
    else if ((network_step == NET_STEP_CONNECT) &&
             (network_conn_state >= 0))
    {
        (void)snprintf(network_last_error, sizeof(network_last_error),
                       "CONN:%d", network_conn_state);
        Network_GoToRetry(now_ms, stage, network_last_error);
    }
    else if ((network_step == NET_STEP_REGISTRATION) &&
             (network_reg_status >= 0))
    {
        (void)snprintf(network_last_error, sizeof(network_last_error),
                       "REG:%d", network_reg_status);
        Network_GoToRetry(now_ms, stage, network_last_error);
    }
    else
    {
        Network_GoToRetry(now_ms, stage, "STEP FAIL");
    }
}

void NetworkManager_Init(UART_HandleTypeDef *uart, bool enabled)
{
    const uint32_t now_ms = HAL_GetTick();

    ATClient_Init(&network_at, uart);
    network_enabled = enabled;
    network_response_length = 0U;
    network_urc_scan_offset = 0U;
    network_command_pending = false;
    network_csq = -1;
    network_reg_status = -1;
    network_conn_state = -1;
    network_cme_error = -1;
    network_publish_message_id = -1;
    network_seen_uart_errors = 0U;
    network_seen_overflows = 0U;
    network_ack_head = 0U;
    network_ack_tail = 0U;
    network_ack_count = 0U;
    network_status_pending = false;
    network_config_pending = false;
    network_alarm_pending = false;
    network_publish_kind = NETWORK_PUBLISH_NONE;
    network_status_part = 0U;
    network_config_part = 0U;
    Network_CopyText(network_publish_label, sizeof(network_publish_label), "--");
    Network_SetPublishTrace("--", "IDLE");
    network_last_alarm_flags = AppAlarm_GetFlags();
    network_last_mode = AppState_GetMode();
    network_last_run_state = AppState_GetRunState();
    network_last_pump = Relay_IsOn();
    network_last_muted = AppAlarm_IsMuted();
    network_last_config = *AppConfig_GetActive();
    network_last_message_valid = false;
    network_last_message_id = 0U;
    network_last_ack_result[0] = '\0';
    network_last_ack_reason[0] = '\0';
    network_failed_stage = NETWORK_STAGE_OFF;
    Network_SetError("NONE");

    if (!enabled)
    {
        network_state = NETWORK_STATE_DISABLED;
        Network_EnterStep(NET_STEP_BOOT, now_ms);
        return;
    }

    if (!Network_ProtocolSelfTest())
    {
        network_state = NETWORK_STATE_ERROR;
        network_failed_stage = NETWORK_STAGE_MQTT_CONFIG;
        Network_SetError("PROTOCOL TEST");
        return;
    }

    if (ATClient_StartReceive(&network_at) != HAL_OK)
    {
        network_state = NETWORK_STATE_RETRY_WAIT;
        network_step = NET_STEP_RETRY;
        network_failed_stage = NETWORK_STAGE_UART;
        network_next_action_ms = now_ms + NETWORK_RETRY_MS;
        Network_SetError("UART RX");
        return;
    }

    network_state = NETWORK_STATE_CONNECTING;
    Network_EnterStep(NET_STEP_BOOT, now_ms);
    network_next_action_ms = now_ms + NETWORK_BOOT_WAIT_MS;
}

void NetworkManager_Update(void)
{
    const uint32_t now_ms = HAL_GetTick();
    const char *command;
    uint32_t uart_errors;
    uint32_t overflows;
    int online_state;
    bool response_ok;

    if (!network_enabled || (network_state == NETWORK_STATE_ERROR))
    {
        return;
    }

    (void)ATClient_EnsureReceive(&network_at);
    uart_errors = ATClient_GetErrorCount(&network_at);
    overflows = ATClient_GetOverflowCount(&network_at);
    if ((uart_errors != network_seen_uart_errors) ||
        (overflows != network_seen_overflows))
    {
        network_seen_uart_errors = uart_errors;
        network_seen_overflows = overflows;
        Network_GoToRetry(now_ms, NETWORK_STAGE_UART,
                          (overflows != 0U) ? "RX OVERFLOW" : "UART ERROR");
        return;
    }

    Network_ReadResponse();
    Network_CapturePublishMessageId();
    /* Clean-session reconnects must not execute stale/downlink data before
     * both subscriptions have completed and the manager is fully online. */
    if (network_state == NETWORK_STATE_ONLINE)
    {
        Network_ProcessInboundUrcs(now_ms);
    }
    Network_DetectLocalChanges();

    if (network_step == NET_STEP_BOOT)
    {
        if (Network_TimeReached(now_ms, network_next_action_ms))
        {
            Network_EnterStep(NET_STEP_CLEAN, now_ms);
        }
        return;
    }

    if (network_step == NET_STEP_RETRY)
    {
        if (Network_TimeReached(now_ms, network_next_action_ms))
        {
            network_state = NETWORK_STATE_CONNECTING;
            Network_EnterStep(NET_STEP_CLEAN, now_ms);
        }
        return;
    }

    if (network_step == NET_STEP_ONLINE)
    {
        online_state =
            ML307C_MQTT_ParseConnectionState(network_response,
                                              ML307C_CONNECT_ID);
        if ((online_state >= 2) ||
            ML307C_MQTT_HasTimeout(network_response))
        {
            network_conn_state = online_state;
            Network_GoToRetry(now_ms, NETWORK_STAGE_MQTT_CONNECT,
                              "LINK LOST");
        }
        else
        {
            if (Network_TimeReached(now_ms, network_next_action_ms))
            {
                network_status_pending = true;
                network_next_action_ms = now_ms +
                                         NETWORK_PUBLISH_PERIOD_MS;
            }
            if ((network_ack_count > 0U) || network_alarm_pending ||
                network_config_pending || network_status_pending)
            {
                if (Network_PreparePublish())
                {
                    Network_EnterStep(NET_STEP_PUBLISH, now_ms);
                }
                else
                {
                    Network_SetError("ENCODE FAIL");
                    network_state = NETWORK_STATE_ERROR;
                }
            }
            else if (network_response_length > 800U)
            {
                Network_ClearResponse();
            }
        }
        return;
    }

    if (!network_command_pending)
    {
        if (!Network_TimeReached(now_ms, network_not_before_ms))
        {
            return;
        }
        command = Network_CommandForStep();
        if ((command == 0) || !Network_SendCommand(command, now_ms))
        {
            Network_GoToRetry(now_ms, Network_StageForStep(network_step),
                              "TX ERROR");
        }
        return;
    }

    if (Network_ResponseFinished())
    {
        network_command_pending = false;
        response_ok = Network_Evaluate();
        Network_HandleStepResult(now_ms, response_ok, false);
    }
    else if (Network_TimeReached(now_ms,
                                 network_step_started_ms +
                                 Network_TimeoutMs()))
    {
        network_command_pending = false;
        response_ok = Network_Evaluate();
        Network_HandleStepResult(now_ms, response_ok, true);
    }
}

Network_State_t NetworkManager_GetState(void)
{
    return network_state;
}

bool NetworkManager_IsEnabled(void)
{
    return network_enabled;
}

bool NetworkManager_IsOnline(void)
{
    return (network_state == NETWORK_STATE_ONLINE);
}

int NetworkManager_GetSignalQuality(void)
{
    return network_csq;
}

const char *NetworkManager_GetLastError(void)
{
    return network_last_error;
}

Network_Stage_t NetworkManager_GetStage(void)
{
    if (!network_enabled)
    {
        return NETWORK_STAGE_OFF;
    }
    if (network_step == NET_STEP_RETRY)
    {
        return network_failed_stage;
    }
    return Network_StageForStep(network_step);
}

Network_Stage_t NetworkManager_GetFailedStage(void)
{
    return network_failed_stage;
}

static const char *Network_StageCode(Network_Stage_t stage)
{
    switch (stage)
    {
        case NETWORK_STAGE_BOOT:          return "BOT";
        case NETWORK_STAGE_AT:            return "AT";
        case NETWORK_STAGE_SIM:           return "SIM";
        case NETWORK_STAGE_SIGNAL:        return "SIG";
        case NETWORK_STAGE_REGISTER:      return "REG";
        case NETWORK_STAGE_ATTACH:        return "DAT";
        case NETWORK_STAGE_MQTT_CONFIG:   return "CFG";
        case NETWORK_STAGE_MQTT_CONNECT:  return "MQ";
        case NETWORK_STAGE_SUBSCRIBE:     return "SUB";
        case NETWORK_STAGE_PUBLISH:       return "PUB";
        case NETWORK_STAGE_ONLINE:        return "ON";
        case NETWORK_STAGE_UART:          return "UART";
        case NETWORK_STAGE_OFF:
        default:                          return "OFF";
    }
}

static const char *Network_RetryCode(Network_Stage_t stage)
{
    switch (stage)
    {
        case NETWORK_STAGE_AT:            return "RAT";
        case NETWORK_STAGE_SIM:           return "RSM";
        case NETWORK_STAGE_SIGNAL:        return "RSG";
        case NETWORK_STAGE_REGISTER:      return "RRG";
        case NETWORK_STAGE_ATTACH:        return "RDT";
        case NETWORK_STAGE_MQTT_CONFIG:   return "RCF";
        case NETWORK_STAGE_MQTT_CONNECT:  return "RMQ";
        case NETWORK_STAGE_SUBSCRIBE:     return "RSB";
        case NETWORK_STAGE_PUBLISH:       return "RPB";
        case NETWORK_STAGE_UART:          return "RUT";
        default:                          return "RTY";
    }
}

const char *NetworkManager_GetDisplayCode(void)
{
    if (network_state == NETWORK_STATE_DISABLED)
    {
        return "OFF";
    }
    if ((network_state == NETWORK_STATE_ONLINE) &&
        (network_step == NET_STEP_PUBLISH))
    {
        return "PUB";
    }
    if (network_state == NETWORK_STATE_ONLINE)
    {
        return "ON";
    }
    if (network_state == NETWORK_STATE_ERROR)
    {
        return "ERR";
    }
    if (network_state == NETWORK_STATE_RETRY_WAIT)
    {
        return Network_RetryCode(network_failed_stage);
    }
    return Network_StageCode(NetworkManager_GetStage());
}

bool NetworkManager_IsPublishing(void)
{
    return (network_state == NETWORK_STATE_ONLINE) &&
           (network_step == NET_STEP_PUBLISH);
}

const char *NetworkManager_GetPublishTrace(void)
{
    return network_publish_trace;
}

uint32_t NetworkManager_GetUartErrorCount(void)
{
    return ATClient_GetErrorCount(&network_at);
}

uint32_t NetworkManager_GetRxOverflowCount(void)
{
    return ATClient_GetOverflowCount(&network_at);
}

HAL_StatusTypeDef NetworkManager_SendRawAT(const char *command)
{
    if (!network_enabled)
    {
        return HAL_ERROR;
    }
    return ATClient_Send(&network_at, command);
}

void NetworkManager_OnUartRxComplete(UART_HandleTypeDef *uart)
{
    ATClient_OnRxComplete(&network_at, uart);
}

void NetworkManager_OnUartError(UART_HandleTypeDef *uart)
{
    ATClient_OnError(&network_at, uart);
}
