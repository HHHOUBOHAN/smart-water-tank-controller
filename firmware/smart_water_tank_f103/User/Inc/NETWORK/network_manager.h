#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <stdbool.h>
#include "stm32f1xx_hal.h"

typedef enum
{
    NETWORK_STATE_DISABLED = 0,
    NETWORK_STATE_CONNECTING,
    NETWORK_STATE_ONLINE,
    NETWORK_STATE_RETRY_WAIT,
    NETWORK_STATE_ERROR
} Network_State_t;

typedef enum
{
    NETWORK_STAGE_OFF = 0,
    NETWORK_STAGE_BOOT,
    NETWORK_STAGE_AT,
    NETWORK_STAGE_SIM,
    NETWORK_STAGE_SIGNAL,
    NETWORK_STAGE_REGISTER,
    NETWORK_STAGE_ATTACH,
    NETWORK_STAGE_MQTT_CONFIG,
    NETWORK_STAGE_MQTT_CONNECT,
    NETWORK_STAGE_SUBSCRIBE,
    NETWORK_STAGE_PUBLISH,
    NETWORK_STAGE_ONLINE,
    NETWORK_STAGE_UART
} Network_Stage_t;

void NetworkManager_Init(UART_HandleTypeDef *uart, bool enabled);
void NetworkManager_Update(void);
Network_State_t NetworkManager_GetState(void);
bool NetworkManager_IsEnabled(void);
bool NetworkManager_IsOnline(void);
int NetworkManager_GetSignalQuality(void);
const char *NetworkManager_GetLastError(void);
Network_Stage_t NetworkManager_GetStage(void);
Network_Stage_t NetworkManager_GetFailedStage(void);
const char *NetworkManager_GetDisplayCode(void);
bool NetworkManager_IsPublishing(void);
const char *NetworkManager_GetPublishTrace(void);
uint32_t NetworkManager_GetUartErrorCount(void);
uint32_t NetworkManager_GetRxOverflowCount(void);
HAL_StatusTypeDef NetworkManager_SendRawAT(const char *command);
void NetworkManager_OnUartRxComplete(UART_HandleTypeDef *uart);
void NetworkManager_OnUartError(UART_HandleTypeDef *uart);

#endif /* NETWORK_MANAGER_H */
