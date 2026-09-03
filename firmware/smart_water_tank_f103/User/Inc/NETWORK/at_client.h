#ifndef NETWORK_AT_CLIENT_H
#define NETWORK_AT_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f1xx_hal.h"

#define AT_CLIENT_RX_BUFFER_SIZE  1024U

typedef struct
{
    UART_HandleTypeDef *uart;
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint32_t overflow_count;
    volatile uint32_t error_count;
    volatile uint32_t last_uart_error;
    uint8_t irq_byte;
    uint8_t buffer[AT_CLIENT_RX_BUFFER_SIZE];
    volatile bool receiving;
} ATClient_t;

void ATClient_Init(ATClient_t *client, UART_HandleTypeDef *uart);
HAL_StatusTypeDef ATClient_StartReceive(ATClient_t *client);
HAL_StatusTypeDef ATClient_Send(ATClient_t *client, const char *command);
bool ATClient_ReadByte(ATClient_t *client, uint8_t *byte);
void ATClient_Clear(ATClient_t *client);
HAL_StatusTypeDef ATClient_EnsureReceive(ATClient_t *client);
void ATClient_OnRxComplete(ATClient_t *client, UART_HandleTypeDef *uart);
void ATClient_OnError(ATClient_t *client, UART_HandleTypeDef *uart);
uint32_t ATClient_GetOverflowCount(const ATClient_t *client);
uint32_t ATClient_GetErrorCount(const ATClient_t *client);

#endif /* NETWORK_AT_CLIENT_H */
