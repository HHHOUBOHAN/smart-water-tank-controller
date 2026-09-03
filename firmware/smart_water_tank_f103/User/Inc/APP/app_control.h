#ifndef APP_CONTROL_H
#define APP_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

#include "APP/app_command.h"

void AppControl_Init(uint32_t now_ms);
void AppControl_HandleCommand(const AppCommand_t *command, uint32_t now_ms);
void AppControl_Process(uint32_t now_ms);
const char *AppControl_GetNoticeText(void);
bool AppControl_CanStart(void);
const char *AppControl_GetStartDeniedReason(void);

#endif /* APP_CONTROL_H */
