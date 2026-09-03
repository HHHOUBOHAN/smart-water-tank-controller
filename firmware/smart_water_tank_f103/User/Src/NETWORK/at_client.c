#include "NETWORK/at_client.h"

#include <string.h>

void ATClient_Init(ATClient_t *client, UART_HandleTypeDef *uart)
{
    if (client == 0)
    {
        return;
    }
    client->uart = uart;
    client->head = 0U;
    client->tail = 0U;
    client->overflow_count = 0U;
    client->error_count = 0U;
    client->last_uart_error = HAL_UART_ERROR_NONE;
    client->irq_byte = 0U;
    client->receiving = false;
}

HAL_StatusTypeDef ATClient_StartReceive(ATClient_t *client)
{
    HAL_StatusTypeDef status;

    if ((client == 0) || (client->uart == 0))
    {
        return HAL_ERROR;
    }
    status = HAL_UART_Receive_IT(client->uart, &client->irq_byte, 1U);
    client->receiving = (status == HAL_OK);
    return status;
}

HAL_StatusTypeDef ATClient_Send(ATClient_t *client, const char *command)
{
    size_t length;

    if ((client == 0) || (client->uart == 0) || (command == 0))
    {
        return HAL_ERROR;
    }
    length = strlen(command);
    if (length > 0xFFFFU)
    {
        return HAL_ERROR;
    }
    return HAL_UART_Transmit(client->uart,
                             (uint8_t *)command,
                             (uint16_t)length,
                             500U);
}

void ATClient_Clear(ATClient_t *client)
{
    if (client == 0)
    {
        return;
    }

    __disable_irq();
    client->tail = client->head;
    __enable_irq();
}

HAL_StatusTypeDef ATClient_EnsureReceive(ATClient_t *client)
{
    HAL_StatusTypeDef status;

    if ((client == 0) || (client->uart == 0))
    {
        return HAL_ERROR;
    }
    if (client->receiving &&
        (client->uart->RxState == HAL_UART_STATE_BUSY_RX))
    {
        return HAL_OK;
    }

    client->receiving = false;
    status = HAL_UART_Receive_IT(client->uart, &client->irq_byte, 1U);
    client->receiving = (status == HAL_OK);
    return status;
}

bool ATClient_ReadByte(ATClient_t *client, uint8_t *byte)
{
    if ((client == 0) || (byte == 0) || (client->tail == client->head))
    {
        return false;
    }
    *byte = client->buffer[client->tail];
    client->tail = (uint16_t)((client->tail + 1U) % AT_CLIENT_RX_BUFFER_SIZE);
    return true;
}

void ATClient_OnRxComplete(ATClient_t *client, UART_HandleTypeDef *uart)
{
    uint16_t next_head;

    if ((client == 0) || (uart == 0) || (client->uart != uart) ||
        !client->receiving)
    {
        return;
    }

    next_head = (uint16_t)((client->head + 1U) % AT_CLIENT_RX_BUFFER_SIZE);
    if (next_head == client->tail)
    {
        client->overflow_count++;
    }
    else
    {
        client->buffer[client->head] = client->irq_byte;
        client->head = next_head;
    }
    client->receiving = false;
    if (HAL_UART_Receive_IT(client->uart, &client->irq_byte, 1U) == HAL_OK)
    {
        client->receiving = true;
    }
}

void ATClient_OnError(ATClient_t *client, UART_HandleTypeDef *uart)
{
    if ((client == 0) || (uart == 0) || (client->uart != uart))
    {
        return;
    }

    client->last_uart_error = HAL_UART_GetError(uart);
    client->error_count++;
    client->receiving = false;
    (void)ATClient_EnsureReceive(client);
}

uint32_t ATClient_GetOverflowCount(const ATClient_t *client)
{
    return (client == 0) ? 0U : client->overflow_count;
}

uint32_t ATClient_GetErrorCount(const ATClient_t *client)
{
    return (client == 0) ? 0U : client->error_count;
}
