#ifndef PROTOCOL_COMMAND_CODEC_H
#define PROTOCOL_COMMAND_CODEC_H

#include <stdbool.h>
#include <stdint.h>

#include "APP/app_config.h"

typedef enum
{
    REMOTE_COMMAND_INVALID = 0,
    REMOTE_COMMAND_START,
    REMOTE_COMMAND_STOP,
    REMOTE_COMMAND_AUTO,
    REMOTE_COMMAND_MANUAL,
    REMOTE_COMMAND_MUTE,
    REMOTE_COMMAND_STATUS
} RemoteCommandType_t;

bool CommandCodec_ParseCommand(const char *json,
                               uint32_t *message_id,
                               RemoteCommandType_t *command);
bool CommandCodec_ParseConfig(const char *json,
                              uint32_t *message_id,
                              AppConfigData_t *config);

#endif /* PROTOCOL_COMMAND_CODEC_H */
