#ifndef RELAY_H
#define RELAY_H
#include "stdbool.h"
//继电器状态
typedef enum
{
    RELAY_STATE_OFF = 0,
    RELAY_STATE_ON  = 1
} RelayState;


//设置状态
void Relay_SetState(RelayState state);

//初始化
void Relay_Init(void);

//开启
void Relay_On(void);

//关闭
void Relay_Off(void);

//获取状态
RelayState Relay_GetState(void);

//判断是否开启
bool Relay_IsOn(void);
 
#endif
