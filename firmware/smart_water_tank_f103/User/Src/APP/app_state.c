#include "APP/app_state.h"
#include <stdbool.h>
#include <stdint.h>


static AppStateData_t app_state;

//判断工作模式枚举值是否合法
static bool AppState_IsModeValid(AppMode_t mode)
{
    return ((uint32_t)mode < (uint32_t)APP_MODE_COUNT);//有符号 int 会被转成无符号
}

//判断运行状态枚举值是否合法
static bool  AppState_IsRunStateValid(AppRunState_t run_state)
{
    return ((uint32_t)run_state<(uint32_t)APP_RUN_STATE_COUNT);
}

//判断oled枚举值是否合法
static bool AppState_IsPageValid(AppPage_t page)
{
    return ((uint32_t)page < (uint32_t)APP_PAGE_COUNT);
}


//初始化
void AppState_Init(uint32_t now_ms)
{
    app_state.mode=APP_MODE_AUTO;
    app_state.run_state=APP_RUN_BOOT_WAIT_VALID;
    app_state.page= APP_PAGE_MAIN;
    app_state.pump_command_on=false;
    app_state.pump_started_at_ms=now_ms;

}

//设置工作模式
void AppState_SetMode(AppMode_t mode)
{
    if(!AppState_IsModeValid(mode))
    {
        return;
    }
    app_state.mode=mode;
}

//读取当前工作模式
AppMode_t AppState_GetMode(void)
{
    return app_state.mode;
}

//设置整机状态
void AppState_SetRunState(AppRunState_t run_state)
{
    if(!AppState_IsRunStateValid(run_state))
    {
        return;
    }
    app_state.run_state=run_state;
}

//读取整机状态
AppRunState_t AppState_GetRunState(void)
{
    return app_state.run_state;
}

//切换OLED显示页面
void AppState_SetPage(AppPage_t page)
{
    if(!AppState_IsPageValid(page))
    {
        return;
    }
    app_state.page=page;
}


//读取OLED状态
AppPage_t AppState_GetPage(void)
{
    return app_state.page;
}

//设置水泵开关指令。从关闭变成开启时，自动记录启动时间。
void AppState_SetPumpCommand(bool on, uint32_t now_ms)//true=开启水泵，false=关闭水泵
{
    if (on && !app_state.pump_command_on)
    {
        app_state.pump_started_at_ms = now_ms;
    }
    app_state.pump_command_on = on;
}

//读取当前水泵开关指令。
bool AppState_GetPumpCommand(void)
{
    return app_state.pump_command_on;
}

//读取本次水泵启动时间。
uint32_t AppState_GetPumpStartedAtMs(void)
{
    return app_state.pump_started_at_ms;
}

//获取整个APP状态的只读指针。外部模块可以读取，但不能直接修改。
const AppStateData_t *AppState_GetData(void)
{
    return &app_state;
}

