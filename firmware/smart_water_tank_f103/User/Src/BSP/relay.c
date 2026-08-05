#include "BSP/relay.h"
#include "BSP/board.h"


//定义初始状态
static RelayState relay_state = RELAY_STATE_OFF;

//将逻辑电平转换为物理GPIO电平
static GPIO_PinState Relay_StateToPinLevel(RelayState state)
{
    if(state == RELAY_STATE_ON)
    {
        return BOARD_RELAY_ACTIVE_LEVEL;
    }
    return BOARD_RELAY_INACTIVE_LEVEL;
}

//设置状态
void Relay_SetState(RelayState state)
{
    GPIO_PinState pin_level;

    if((state != RELAY_STATE_OFF) &&
       (state != RELAY_STATE_ON))
    {
        return;
    }

    pin_level = Relay_StateToPinLevel(state);

    HAL_GPIO_WritePin(
        BOARD_RELAY_GPIO_PORT,
        BOARD_RELAY_GPIO_PIN,
        pin_level
    );

    relay_state = state;
}



//初始化
void Relay_Init(void)
{
    Relay_SetState(RELAY_STATE_OFF);
}


//开启
void Relay_On(void)
{
    Relay_SetState(RELAY_STATE_ON);
}

//关闭
void Relay_Off(void)
{
    Relay_SetState(RELAY_STATE_OFF); 
}
 

//获取状态
RelayState Relay_GetState(void)
{
    return relay_state;
}

//判断是否开启
bool Relay_IsOn(void)
{
    return (relay_state == RELAY_STATE_ON);
}
 