#ifndef LED_H
#define LED_H
#include <stdbool.h>

typedef enum
{
    LED_RUN = 0,
    LED_FAULT,

    LED_COUNT
}LED_TypeDef;

typedef enum
{
    LED_STATE_OFF = 0,
    LED_STATE_ON
} LED_State_t;

//初始化LED
void LED_Init(void);

//灯亮
void LED_On(LED_TypeDef led);

//灯灭
void LED_Off(LED_TypeDef led);

//设置状态
void LED_SetState(LED_TypeDef led, LED_State_t state);

//翻转状态
void LED_Toggle(LED_TypeDef led);

//获取当前状态
bool LED_GetState(LED_TypeDef led, LED_State_t *state);


#endif 