#ifndef BUZZER_H
#define BUZZER_H

typedef enum
{
    BUZZER_STATE_OFF = 0,
    BUZZER_STATE_ON
} Buzzer_State_t;


//初始化
void Buzzer_Init(void);

//设置蜂鸣器状态
void Buzzer_SetState(Buzzer_State_t state);

//蜂鸣器开
void Buzzer_On(void);

//蜂鸣器关
void Buzzer_Off(void);

//蜂鸣器状态切换
void Buzzer_Toggle(void);

//获取蜂鸣器状态
Buzzer_State_t Buzzer_GetState(Buzzer_State_t *state);

#endif