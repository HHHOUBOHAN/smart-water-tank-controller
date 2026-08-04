#include "led.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState on_level;
    GPIO_PinState off_level;
} LED_HardwareTypeDef;

//判断LED编号是否合法
static bool LED_IsValid(LED_TypeDef led)
{
    return ( (uint32_t)led < (uint32_t)LED_COUNT );
}
 
static const LED_HardwareTypeDef led_table[LED_COUNT] =
{
    [LED_RUN] =
    {
        RUN_LED_GPIO_Port,
        RUN_LED_Pin,
        GPIO_PIN_SET,
        GPIO_PIN_RESET
    },
    [LED_FAULT] =
    {
        FAULT_LED_GPIO_Port,
        FAULT_LED_Pin,
        GPIO_PIN_SET,
        GPIO_PIN_RESET
    }
};


//将逻辑状态转换成物理GPIO电平
static GPIO_PinState LED_StateToPinLevel( LED_TypeDef led, LED_State_t state)
{
    if (state == LED_STATE_ON)
    {
        return led_table[led].on_level;
    }
    
    return led_table[led].off_level;
}

//初始化
void LED_Init(void)
{
    LED_SetState(
        LED_RUN,
        LED_STATE_OFF
    );

    LED_SetState(
        LED_FAULT,
        LED_STATE_OFF
    );
}


//设置状态
void LED_SetState(
    LED_TypeDef led,
    LED_State_t state
)
{
    GPIO_PinState pin_level;

    if (!LED_IsValid(led))
    {
        return;
    }

    if ((state != LED_STATE_OFF) &&
        (state != LED_STATE_ON))
    {
        return;
    }

    pin_level = LED_StateToPinLevel(
        led,
        state
    );

    HAL_GPIO_WritePin(
        led_table[led].port,
        led_table[led].pin,
        pin_level
    );
}

//灯亮
void LED_On(LED_TypeDef led)
{
    LED_SetState(
        led,
        LED_STATE_ON
    );
}

//灯灭
void LED_Off(LED_TypeDef led)
{
    LED_SetState(
        led,
        LED_STATE_OFF
    );
}
//获取当前状态
bool LED_GetState(
    LED_TypeDef led,
    LED_State_t *state
)
{
    GPIO_PinState gpio_level;

    if(!LED_IsValid(led))
    {
        return false;
    }

    if(state == NULL)
    {
        return false;
    }
    gpio_level = HAL_GPIO_ReadPin(led_table[led].port, led_table[led].pin);

    if(gpio_level == led_table[led].on_level)
    {
        *state = LED_STATE_ON;
    }
    else
    {
        *state = LED_STATE_OFF;
    }
    return true;
}

//翻转状态
void LED_Toggle(LED_TypeDef led)
{
    LED_State_t current_state;

    if(!LED_GetState(led,&current_state))
    {
        return;
    }

    if(current_state==LED_STATE_ON)
    {
        LED_Off(led);
    }
    else
    {
        LED_On(led);
    }
}










  