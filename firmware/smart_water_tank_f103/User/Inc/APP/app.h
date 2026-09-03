#ifndef APP_H
#define APP_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

void App_Init(void);
void App_Run(void);
void App_OnGpioExti(uint16_t gpio_pin);
void App_OnUartRxComplete(UART_HandleTypeDef *uart);
void App_OnUartError(UART_HandleTypeDef *uart);

#endif /* APP_H */
