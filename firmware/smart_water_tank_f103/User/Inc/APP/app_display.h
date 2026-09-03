#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

void AppDisplay_Init(uint32_t now_ms);
void AppDisplay_Process(uint32_t now_ms);
void AppDisplay_RequestRefresh(void);
bool AppDisplay_IsReady(void);

#endif /* APP_DISPLAY_H */
