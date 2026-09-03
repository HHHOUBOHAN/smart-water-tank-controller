#ifndef APP_COMMAND_H
#define APP_COMMAND_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    APP_COMMAND_NONE = 0,
    APP_COMMAND_MODE_TOGGLE,
    APP_COMMAND_START,
    APP_COMMAND_STOP,
    APP_COMMAND_MUTE_TOGGLE,
    APP_COMMAND_ENTER_CONFIG,
    APP_COMMAND_CONFIG_NEXT,
    APP_COMMAND_CONFIG_INCREASE,
    APP_COMMAND_CONFIG_DECREASE,
    APP_COMMAND_CONFIG_SAVE,
    APP_COMMAND_CONFIG_CANCEL,
    APP_COMMAND_PRESSURE_TARE,
    APP_COMMAND_COUNT
} AppCommandType_t;

typedef enum
{
    APP_COMMAND_SOURCE_KEY = 0,
    APP_COMMAND_SOURCE_MQTT,
    APP_COMMAND_SOURCE_SYSTEM,
    APP_COMMAND_SOURCE_COUNT
} AppCommandSource_t;

typedef struct
{
    AppCommandType_t type;
    AppCommandSource_t source;
    uint32_t timestamp_ms;
} AppCommand_t;

void AppCommand_Init(void);
void AppCommand_Update(void);
bool AppCommand_Post(AppCommandType_t type,
                     AppCommandSource_t source,
                     uint32_t timestamp_ms);
bool AppCommand_Get(AppCommand_t *command);
bool AppCommand_TakeStopLatch(void);
bool AppCommand_HasPending(void);
uint8_t AppCommand_GetPendingCount(void);
uint32_t AppCommand_GetLostCount(void);
uint32_t AppCommand_GetCancelledCount(void);

#endif /* APP_COMMAND_H */
