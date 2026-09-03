#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "APP/app_config.h"
#include "COMMON/text_parser.h"
#include "NETWORK/ml307c_at.h"
#include "NETWORK/ml307c_mqtt.h"
#include "PROTOCOL/command_codec.h"

static void TestTextParser(void)
{
    const char *cursor;
    uint32_t unsigned_value;
    int32_t signed_value;

    cursor = " 4294967295,";
    assert(TextParser_ParseUInt32(&cursor, &unsigned_value));
    assert(unsigned_value == UINT32_MAX);
    assert(TextParser_ConsumeChar(&cursor, ','));

    cursor = "4294967296";
    assert(!TextParser_ParseUInt32(&cursor, &unsigned_value));

    cursor = " -2147483648";
    assert(TextParser_ParseInt32(&cursor, &signed_value));
    assert(signed_value == INT32_MIN);

    cursor = "2147483648";
    assert(!TextParser_ParseInt32(&cursor, &signed_value));
}

static void TestAtParser(void)
{
    assert(ML307C_AT_ParseCmeError("\r\n+CME ERROR: 515\r\n") == 515);
    assert(ML307C_AT_ParseSignalQuality("\r\n+CSQ: 30,99\r\n") == 30);
    assert(ML307C_AT_ParseRegistration("\r\n+CEREG: 0,1\r\n") == 1);
    assert(ML307C_AT_ParseRegistration("\r\n+CGREG: 5\r\n") == 5);
}

static void TestMqttParser(void)
{
    char topic[64];
    char payload[96];
    const char json[] = "{\"id\":7,\"cmd\":\"STOP\"}";
    char line[192];

    assert(ML307C_MQTT_ParseConnectionState("\"conn\",0,1", 0) == 1);
    assert(ML307C_MQTT_HasSubAck("\"suback\",0,23,1", 0));
    assert(!ML307C_MQTT_HasSubAck("\"suback\",0,23,128", 0));
    assert(ML307C_MQTT_HasPubAckForMessage("\"puback\",0,42,0", 0, 42));
    assert(ML307C_MQTT_ParsePublishMessageId("+MQTTPUB: 0,42,17", 0) == 42);

    (void)snprintf(line, sizeof(line),
                   "\"publish\",0,9,\"water_tank/water_tank_001/command\",%u,%u,%s",
                   (unsigned int)strlen(json), (unsigned int)strlen(json), json);
    assert(ML307C_MQTT_ExtractPublish(line, 0,
                                     topic, sizeof(topic),
                                     payload, sizeof(payload)));
    assert(strcmp(topic, "water_tank/water_tank_001/command") == 0);
    assert(strcmp(payload, json) == 0);
}

static void TestCommandCodec(void)
{
    uint32_t message_id;
    RemoteCommandType_t command;
    AppConfigData_t config;

    assert(CommandCodec_ParseCommand("{\"id\":101,\"cmd\":\"MANUAL\"}",
                                     &message_id, &command));
    assert(message_id == 101U);
    assert(command == REMOTE_COMMAND_MANUAL);

    assert(CommandCodec_ParseConfig(
        "{\"id\":102,\"low_level\":30,\"high_level\":80,"
        "\"pressure_trip\":120000,\"pressure_recover\":-1000,"
        "\"max_run_min\":20}",
        &message_id, &config));
    assert(config.low_level_percent == 30U);
    assert(config.high_level_percent == 80U);
    assert(config.pressure_trip_delta_raw == 120000);
    assert(config.pressure_recover_delta_raw == -1000);
    assert(config.max_run_minutes == 20U);
}

int main(void)
{
    TestTextParser();
    TestAtParser();
    TestMqttParser();
    TestCommandCodec();
    puts("protocol parser tests passed");
    return 0;
}
