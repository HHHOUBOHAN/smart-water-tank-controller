#ifndef APP_STATE_H
#define APP_STATE_H
#include "stdint.h"
#include "stdbool.h"
 

//工作模式
typedef enum{
    APP_MODE_AUTO = 0,   
    APP_MODE_MANUAL,  
    APP_MODE_COUNT
}AppMode_t;

//整机运行状态
typedef enum{
    APP_RUN_BOOT_WAIT_VALID = 0, // 上电等待状态
    APP_RUN_IDLE,                // 正常待机状态 
    APP_RUN_RUNNING,             // 运行状态 
    APP_RUN_AUTO_PAUSED,         //自动模式暂停状态
    APP_RUN_FAULT_LOCKED,         // 故障锁定状态
    APP_RUN_STATE_COUNT
}AppRunState_t;


//oled显示状态
typedef enum{
    APP_PAGE_MAIN = 0,    // 主页面 
    APP_PAGE_CONFIG,      // 参数设置页面
    APP_PAGE_COUNT     
} AppPage_t;


//APP层全局状态数据集
typedef struct
{
    AppMode_t mode;                // 当前工作模式 
    AppRunState_t run_state;       // 整机运行主状态
    AppPage_t page;                // OLED当前显示页面
    bool pump_command_on;          // 水泵控制指令：true=开启水泵，false=关闭水泵
    uint32_t pump_started_at_ms;   // 水泵启动时刻时间戳(ms)，用于计时、超时保护
} AppStateData_t;

//初始化
void AppState_Init(uint32_t now_ms);

//设置工作模式
void AppState_SetMode(AppMode_t mode);

//读取当前工作模式
AppMode_t AppState_GetMode(void);

//设置整机状态
void AppState_SetRunState(AppRunState_t run_state);

//读取整机状态
AppRunState_t AppState_GetRunState(void); 

//切换OLED显示页面
void AppState_SetPage(AppPage_t page);

//读取OLED状态
AppPage_t AppState_GetPage(void);

//设置水泵开关指令。从关闭变成开启时，自动记录启动时间。
void AppState_SetPumpCommand(bool on, uint32_t now_ms);

//读取当前水泵开关指令。
bool AppState_GetPumpCommand(void);

//读取本次水泵启动时间。
uint32_t AppState_GetPumpStartedAtMs(void);

//获取整个APP状态的只读指针。外部模块可以读取，但不能直接修改。
const AppStateData_t *AppState_GetData(void);


#endif