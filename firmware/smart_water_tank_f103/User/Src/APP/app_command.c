#include "APP/app_command.h"

#include "APP/app_state.h"
#include "BSP/key.h"

#define APP_COMMAND_QUEUE_SIZE  16U

static AppCommand_t app_command_queue[APP_COMMAND_QUEUE_SIZE];
static uint8_t app_command_head;
static uint8_t app_command_tail;
static uint8_t app_command_count;
static uint32_t app_command_lost_count;
static uint32_t app_command_cancelled_count;
static bool app_command_stop_latched;

static bool AppCommand_IsValid(AppCommandType_t type,
                               AppCommandSource_t source)
{
    return ((uint32_t)type > (uint32_t)APP_COMMAND_NONE) &&
           ((uint32_t)type < (uint32_t)APP_COMMAND_COUNT) &&
           ((uint32_t)source < (uint32_t)APP_COMMAND_SOURCE_COUNT);
}

static void AppCommand_ClearPending(void)
{
    app_command_cancelled_count += app_command_count;
    app_command_head = 0U;
    app_command_tail = 0U;
    app_command_count = 0U;
}

void AppCommand_Init(void)
{
    app_command_head = 0U;
    app_command_tail = 0U;
    app_command_count = 0U;
    app_command_lost_count = 0U;
    app_command_cancelled_count = 0U;
    app_command_stop_latched = false;
}

bool AppCommand_Post(AppCommandType_t type,
                     AppCommandSource_t source,
                     uint32_t timestamp_ms)
{
    AppCommand_t *command;

    if (!AppCommand_IsValid(type, source))
    {
        return false;
    }
    if ((type == APP_COMMAND_STOP) &&
        (AppState_GetPage() == APP_PAGE_MAIN))
    {
        AppCommand_ClearPending();
        app_command_stop_latched = true;
        return true;
    }
    if (app_command_count >= APP_COMMAND_QUEUE_SIZE)
    {
        app_command_lost_count++;
        return false;
    }

    command = &app_command_queue[app_command_tail];
    command->type = type;
    command->source = source;
    command->timestamp_ms = timestamp_ms;
    app_command_tail = (uint8_t)((app_command_tail + 1U) %
                                 APP_COMMAND_QUEUE_SIZE);
    app_command_count++;
    return true;
}

static void AppCommand_HandleMain(const Key_Event_t *event)
{
    AppCommandType_t command = APP_COMMAND_NONE;

    if ((event->key == KEY_ID_MODE) &&
        (event->type == KEY_EVENT_SHORT_PRESS))
    {
        command = APP_COMMAND_MODE_TOGGLE;
    }
    else if ((event->key == KEY_ID_MODE) &&
             (event->type == KEY_EVENT_LONG_PRESS))
    {
        command = APP_COMMAND_ENTER_CONFIG;
    }
    else if ((event->key == KEY_ID_START) &&
             (event->type == KEY_EVENT_DOWN))
    {
        command = APP_COMMAND_START;
    }
    else if ((event->key == KEY_ID_STOP) &&
             (event->type == KEY_EVENT_DOWN))
    {
        command = APP_COMMAND_STOP;
    }
    else if ((event->key == KEY_ID_MUTE) &&
             (event->type == KEY_EVENT_SHORT_PRESS))
    {
        command = APP_COMMAND_MUTE_TOGGLE;
    }
    else if ((event->key == KEY_ID_MUTE) &&
             (event->type == KEY_EVENT_LONG_PRESS))
    {
        command = APP_COMMAND_PRESSURE_TARE;
    }

    if (command != APP_COMMAND_NONE)
    {
        (void)AppCommand_Post(command,
                              APP_COMMAND_SOURCE_KEY,
                              event->timestamp_ms);
    }
}

static void AppCommand_HandleConfig(const Key_Event_t *event)
{
    AppCommandType_t command = APP_COMMAND_NONE;

    if ((event->key == KEY_ID_MODE) &&
        (event->type == KEY_EVENT_SHORT_PRESS))
    {
        command = APP_COMMAND_CONFIG_NEXT;
    }
    else if ((event->key == KEY_ID_MODE) &&
             (event->type == KEY_EVENT_LONG_PRESS))
    {
        command = APP_COMMAND_CONFIG_SAVE;
    }
    else if ((event->key == KEY_ID_START) &&
             ((event->type == KEY_EVENT_DOWN) ||
              (event->type == KEY_EVENT_REPEAT)))
    {
        command = APP_COMMAND_CONFIG_INCREASE;
    }
    else if ((event->key == KEY_ID_STOP) &&
             ((event->type == KEY_EVENT_DOWN) ||
              (event->type == KEY_EVENT_REPEAT)))
    {
        command = APP_COMMAND_CONFIG_DECREASE;
    }
    else if ((event->key == KEY_ID_MUTE) &&
             (event->type == KEY_EVENT_SHORT_PRESS))
    {
        command = APP_COMMAND_MUTE_TOGGLE;
    }
    else if ((event->key == KEY_ID_MUTE) &&
             (event->type == KEY_EVENT_LONG_PRESS))
    {
        command = APP_COMMAND_CONFIG_CANCEL;
    }

    if (command != APP_COMMAND_NONE)
    {
        (void)AppCommand_Post(command,
                              APP_COMMAND_SOURCE_KEY,
                              event->timestamp_ms);
    }
}

void AppCommand_Update(void)
{
    Key_Event_t event;

    while (Key_GetEvent(&event))
    {
        if (AppState_GetPage() == APP_PAGE_CONFIG)
        {
            AppCommand_HandleConfig(&event);
        }
        else
        {
            AppCommand_HandleMain(&event);
        }
    }
}

bool AppCommand_Get(AppCommand_t *command)
{
    if ((command == 0) || (app_command_count == 0U))
    {
        return false;
    }
    *command = app_command_queue[app_command_head];
    app_command_head = (uint8_t)((app_command_head + 1U) %
                                 APP_COMMAND_QUEUE_SIZE);
    app_command_count--;
    return true;
}

bool AppCommand_TakeStopLatch(void)
{
    const bool latched = app_command_stop_latched;
    app_command_stop_latched = false;
    return latched;
}

bool AppCommand_HasPending(void)
{
    return (app_command_count != 0U);
}

uint8_t AppCommand_GetPendingCount(void)
{
    return app_command_count;
}

uint32_t AppCommand_GetLostCount(void)
{
    return app_command_lost_count;
}

uint32_t AppCommand_GetCancelledCount(void)
{
    return app_command_cancelled_count;
}
