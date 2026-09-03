#include "PROTOCOL/command_codec.h"

#include "COMMON/text_parser.h"

#include <string.h>

static const char *CommandCodec_FindValue(const char *json, const char *key)
{
    const char *position;
    const char *colon;

    if ((json == 0) || (key == 0))
    {
        return 0;
    }
    position = strstr(json, key);
    if (position == 0)
    {
        return 0;
    }
    colon = strchr(position + strlen(key), ':');
    if (colon == 0)
    {
        return 0;
    }
    colon++;
    while ((*colon == ' ') || (*colon == '\t'))
    {
        colon++;
    }
    return colon;
}

static bool CommandCodec_ParseUInt(const char *json,
                                   const char *key,
                                   uint32_t *value)
{
    const char *position = CommandCodec_FindValue(json, key);

    if ((position == 0) || (value == 0) ||
        !TextParser_ParseUInt32(&position, value))
    {
        return false;
    }
    return true;
}

static bool CommandCodec_ParseInt32(const char *json,
                                    const char *key,
                                    int32_t *value)
{
    const char *position = CommandCodec_FindValue(json, key);

    if ((position == 0) || (value == 0) ||
        !TextParser_ParseInt32(&position, value))
    {
        return false;
    }
    return true;
}

static bool CommandCodec_ParseString(const char *json,
                                     const char *key,
                                     char *value,
                                     size_t value_size)
{
    const char *position = CommandCodec_FindValue(json, key);
    const char *end;
    size_t length;

    if ((position == 0) || (value == 0) || (value_size < 2U) ||
        (*position != '"'))
    {
        return false;
    }
    position++;
    end = strchr(position, '"');
    if (end == 0)
    {
        return false;
    }
    length = (size_t)(end - position);
    if (length >= value_size)
    {
        return false;
    }
    (void)memcpy(value, position, length);
    value[length] = '\0';
    return true;
}

bool CommandCodec_ParseCommand(const char *json,
                               uint32_t *message_id,
                               RemoteCommandType_t *command)
{
    char text[12];

    if ((message_id == 0) || (command == 0) ||
        !CommandCodec_ParseUInt(json, "\"id\"", message_id) ||
        (*message_id == 0U) ||
        !CommandCodec_ParseString(json, "\"cmd\"", text, sizeof(text)))
    {
        return false;
    }

    *command = REMOTE_COMMAND_INVALID;
    if (strcmp(text, "START") == 0)       *command = REMOTE_COMMAND_START;
    else if (strcmp(text, "STOP") == 0)   *command = REMOTE_COMMAND_STOP;
    else if (strcmp(text, "AUTO") == 0)   *command = REMOTE_COMMAND_AUTO;
    else if (strcmp(text, "MANUAL") == 0) *command = REMOTE_COMMAND_MANUAL;
    else if (strcmp(text, "MUTE") == 0)   *command = REMOTE_COMMAND_MUTE;
    else if (strcmp(text, "STATUS") == 0) *command = REMOTE_COMMAND_STATUS;

    return (*command != REMOTE_COMMAND_INVALID);
}

bool CommandCodec_ParseConfig(const char *json,
                              uint32_t *message_id,
                              AppConfigData_t *config)
{
    uint32_t value;

    if ((message_id == 0) || (config == 0) ||
        !CommandCodec_ParseUInt(json, "\"id\"", message_id) ||
        (*message_id == 0U) ||
        !CommandCodec_ParseUInt(json, "\"low_level\"", &value) ||
        (value > 255U))
    {
        return false;
    }
    config->low_level_percent = (uint8_t)value;

    if (!CommandCodec_ParseUInt(json, "\"high_level\"", &value) ||
        (value > 255U))
    {
        return false;
    }
    config->high_level_percent = (uint8_t)value;

    if (!CommandCodec_ParseInt32(json, "\"pressure_trip\"",
                                 &config->pressure_trip_delta_raw) ||
        !CommandCodec_ParseInt32(json, "\"pressure_recover\"",
                                 &config->pressure_recover_delta_raw) ||
        !CommandCodec_ParseUInt(json, "\"max_run_min\"", &value) ||
        (value > 65535U))
    {
        return false;
    }
    config->max_run_minutes = (uint16_t)value;
    return true;
}
