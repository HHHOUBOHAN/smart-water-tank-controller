#include "NETWORK/ml307c_mqtt.h"

#include "COMMON/text_parser.h"
#include "NETWORK/ml307c_at.h"

#include <stdio.h>
#include <string.h>

bool ML307C_MQTT_IsSupported(const char *response)
{
    return (response != 0) &&
           (strstr(response, "+MQTTCFG:") != 0) &&
           !ML307C_AT_HasError(response);
}

int ML307C_MQTT_ParseConnectionState(const char *response, int connect_id)
{
    const char *position;
    int32_t parsed_id;
    int32_t state;

    if (response == 0)
    {
        return -1;
    }
    position = strstr(response, "\"conn\",");
    if (position != 0)
    {
        position += strlen("\"conn\",");
        if (TextParser_ParseInt32(&position, &parsed_id) &&
            TextParser_ConsumeChar(&position, ',') &&
            TextParser_ParseInt32(&position, &state) &&
            (parsed_id == connect_id))
        {
            return (int)state;
        }
    }
    return -1;
}

bool ML307C_MQTT_HasSubAck(const char *response, int connect_id)
{
    const char *position;
    int32_t parsed_id;
    int32_t message_id;
    int32_t code;

    if (response == 0)
    {
        return false;
    }
    position = strstr(response, "\"suback\",");
    if (position == 0)
    {
        return false;
    }
    position += strlen("\"suback\",");
    return TextParser_ParseInt32(&position, &parsed_id) &&
           TextParser_ConsumeChar(&position, ',') &&
           TextParser_ParseInt32(&position, &message_id) &&
           TextParser_ConsumeChar(&position, ',') &&
           TextParser_ParseInt32(&position, &code) &&
           (parsed_id == connect_id) && (code != 128);
}

bool ML307C_MQTT_HasPubAck(const char *response, int connect_id)
{
    return ML307C_MQTT_HasPubAckForMessage(response, connect_id, -1);
}

bool ML307C_MQTT_HasPubAckForMessage(const char *response,
                                     int connect_id,
                                     int message_id)
{
    const char *position;
    int32_t parsed_id;
    int32_t parsed_message_id;
    int32_t duplicate;

    if (response == 0)
    {
        return false;
    }
    position = strstr(response, "\"puback\",");
    if (position == 0)
    {
        return false;
    }
    position += strlen("\"puback\",");
    if (!TextParser_ParseInt32(&position, &parsed_id) ||
        !TextParser_ConsumeChar(&position, ',') ||
        !TextParser_ParseInt32(&position, &parsed_message_id) ||
        !TextParser_ConsumeChar(&position, ',') ||
        !TextParser_ParseInt32(&position, &duplicate))
    {
        return false;
    }
    (void)duplicate;
    return (parsed_id == connect_id) &&
           ((message_id < 0) || (parsed_message_id == message_id));
}

int ML307C_MQTT_ParsePublishMessageId(const char *response,
                                      int connect_id)
{
    const char *position;
    int32_t parsed_id;
    int32_t message_id;
    uint32_t length;

    if (response == 0)
    {
        return -1;
    }
    position = strstr(response, "+MQTTPUB:");
    if (position != 0)
    {
        position += strlen("+MQTTPUB:");
        if (TextParser_ParseInt32(&position, &parsed_id) &&
            TextParser_ConsumeChar(&position, ',') &&
            TextParser_ParseInt32(&position, &message_id) &&
            TextParser_ConsumeChar(&position, ',') &&
            TextParser_ParseUInt32(&position, &length) &&
            (parsed_id == connect_id))
        {
            (void)length;
            return (int)message_id;
        }
    }
    return -1;
}

bool ML307C_MQTT_HasTimeout(const char *response)
{
    return (response != 0) &&
           (strstr(response, "\"timeout\",") != 0);
}

bool ML307C_MQTT_ExtractPublish(const char *line,
                                int connect_id,
                                char *topic,
                                size_t topic_size,
                                char *payload,
                                size_t payload_size)
{
    const char *cursor;
    const char *end;
    int32_t parsed_id;
    int32_t message_id;
    uint32_t total_length;
    uint32_t fragment_length;
    size_t topic_length;

    if ((line == 0) || (topic == 0) || (topic_size < 2U) ||
        (payload == 0) || (payload_size < 2U))
    {
        return false;
    }
    cursor = strstr(line, "\"publish\",");
    if (cursor == 0)
    {
        return false;
    }
    cursor += strlen("\"publish\",");
    if (!TextParser_ParseInt32(&cursor, &parsed_id) ||
        !TextParser_ConsumeChar(&cursor, ',') ||
        !TextParser_ParseInt32(&cursor, &message_id))
    {
        return false;
    }
    if (parsed_id != connect_id)
    {
        return false;
    }
    if (!TextParser_ConsumeChar(&cursor, ',') || (*cursor != '"'))
    {
        return false;
    }
    cursor++;
    end = strchr(cursor, '"');
    if (end == 0)
    {
        return false;
    }
    topic_length = (size_t)(end - cursor);
    if (topic_length >= topic_size)
    {
        return false;
    }
    (void)memcpy(topic, cursor, topic_length);
    topic[topic_length] = '\0';

    cursor = end + 1;
    if (!TextParser_ConsumeChar(&cursor, ',') ||
        !TextParser_ParseUInt32(&cursor, &total_length) ||
        !TextParser_ConsumeChar(&cursor, ',') ||
        !TextParser_ParseUInt32(&cursor, &fragment_length) ||
        !TextParser_ConsumeChar(&cursor, ','))
    {
        return false;
    }

    /* Formal command/config messages are deliberately limited to one URC.
     * Reject fragmented or oversized data instead of executing partial JSON. */
    if ((total_length != fragment_length) ||
        ((size_t)fragment_length >= payload_size) ||
        (strlen(cursor) < (size_t)fragment_length))
    {
        return false;
    }
    (void)memcpy(payload, cursor, (size_t)fragment_length);
    payload[fragment_length] = '\0';
    return true;
}

bool ML307C_MQTT_BuildConnect(char *buffer,
                              size_t buffer_size,
                              int connect_id,
                              const char *host,
                              const char *port,
                              const char *client_id,
                              const char *username,
                              const char *password)
{
    int result;

    if ((buffer == 0) || (buffer_size == 0U) || (host == 0) ||
        (port == 0) || (client_id == 0) || (username == 0) ||
        (password == 0))
    {
        return false;
    }
    result = snprintf(buffer, buffer_size,
                      "AT+MQTTCONN=%d,\"%s\",%s,\"%s\",\"%s\",\"%s\"\r\n",
                      connect_id, host, port, client_id, username, password);
    return (result > 0) && ((size_t)result < buffer_size);
}

bool ML307C_MQTT_BuildSubscribe(char *buffer,
                                size_t buffer_size,
                                int connect_id,
                                const char *topic,
                                int qos)
{
    int result;

    if ((buffer == 0) || (buffer_size == 0U) || (topic == 0))
    {
        return false;
    }
    result = snprintf(buffer, buffer_size,
                      "AT+MQTTSUB=%d,\"%s\",%d\r\n",
                      connect_id, topic, qos);
    return (result > 0) && ((size_t)result < buffer_size);
}

bool ML307C_MQTT_BuildPublish(char *buffer,
                              size_t buffer_size,
                              int connect_id,
                              const char *topic,
                              int qos,
                              const char *payload)
{
    int result;
    size_t payload_length;

    if ((buffer == 0) || (buffer_size == 0U) ||
        (topic == 0) || (payload == 0) || (payload[0] == '\0'))
    {
        return false;
    }
    payload_length = strlen(payload);
    result = snprintf(buffer, buffer_size,
                      "AT+MQTTPUB=%d,\"%s\",%d,0,0,%u,\"%s\"\r\n",
                      connect_id, topic, qos,
                      (unsigned int)payload_length, payload);
    return (result > 0) && ((size_t)result < buffer_size);
}
