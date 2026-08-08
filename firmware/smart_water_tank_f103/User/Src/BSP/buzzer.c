#include "BSP/buzzer.h"
#include "BSP/board.h"
//蜂鸣器当前状态 初始时初始值为OFF 程序刚启动时，软件认为蜂鸣器应当关闭。
static Buzzer_State_t buzzer_state = BUZZER_STATE_OFF;


//将逻辑状态转换成物理GPIO电平
static GPIO_PinState Buzzer_StateToPinLevel( Buzzer_State_t state)
{
    if (state == BUZZER_STATE_ON)
    {
        return BOARD_BUZZER_ACTIVE_LEVEL;
    }

    return BOARD_BUZZER_INACTIVE_LEVEL;
}
//见滚这个

//初始化
void Buzzer_Init(void)
{
    //驱动初始化只负责设置安全状态。
    Buzzer_SetState(BUZZER_STATE_OFF);
}

//设置蜂鸣器状态
void Buzzer_SetState(Buzzer_State_t state)
{
    GPIO_PinState pin_level;// STM32 HAL 库自带的标准枚举类型
    //判断是否是非法值
    if ((state != BUZZER_STATE_OFF) &&
        (state != BUZZER_STATE_ON))
    {
        return;
    }
    //将逻辑状态转换成物理GPIO电平
    pin_level = Buzzer_StateToPinLevel(state);
    //真正操作PB13
    HAL_GPIO_WritePin(
        BOARD_BUZZER_GPIO_PORT,
        BOARD_BUZZER_GPIO_PIN,
        pin_level
    );
    //GPIO写入完成后，记录当前逻辑状态
    buzzer_state = state;
}

//蜂鸣器开
void Buzzer_On(void)
{
    Buzzer_SetState(BUZZER_STATE_ON);
}

//蜂鸣器关
void Buzzer_Off(void)
{
    Buzzer_SetState(BUZZER_STATE_OFF);
}



//蜂鸣器状态切换
void Buzzer_Toggle(void)
{
    if (buzzer_state == BUZZER_STATE_ON)
    {
        Buzzer_Off();
    }
    else
    {
        Buzzer_On();
    }
}

//获取蜂鸣器状态
Buzzer_State_t Buzzer_GetState(Buzzer_State_t *state)
{
    if (state == NULL)
    {
        return BUZZER_STATE_OFF;
    }

    *state = buzzer_state;
    return buzzer_state;
}






 

