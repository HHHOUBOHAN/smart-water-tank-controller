#ifndef APP_INDICATOR_H
#define APP_INDICATOR_H

#include <stdint.h>

void AppIndicator_Init(uint32_t now_ms);
void AppIndicator_Process(uint32_t now_ms);

#endif /* APP_INDICATOR_H */
