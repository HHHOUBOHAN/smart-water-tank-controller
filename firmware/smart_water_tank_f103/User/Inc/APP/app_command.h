#ifndef APP_COMMAND_H
#define APP_COMMAND_H
#include "stdint.h"
#include "stdbool.h"

//APP统一命令类型
typedef enum{
    APP_COMMAND_NONE = 0,

    //主页面命令
    APP_COMMAND_MODE_TOGGLE,       //模式切换
    APP_COMMAND_START,             //启动和恢复自动判断
    APP_COMMAND_STOP,              //停止请求
    APP_COMMAND_MUTE_TOGGLE,       //蜂鸣器消音/解除消音 

    //参数设置命令
    APP_COMMAND_ENTER_CONFIG,      /* 进入参数设置 */
    APP_COMMAND_CONFIG_NEXT,       /* 选择下一项参数 */
    APP_COMMAND_CONFIG_INCREASE,   /* 参数增加 */
    APP_COMMAND_CONFIG_DECREASE,   /* 参数减少 */
    APP_COMMAND_CONFIG_SAVE,       /* 保存参数并退出 */
    APP_COMMAND_CONFIG_CANCEL,     /* 放弃修改并退出 */

    APP_COMMAND_COUNT
}AppCommandType_t;

//命令来源
typedef enum{
    APP_COMMAND_SOURCE_KEY = 0,    /* 本地按键 */
    APP_COMMAND_SOURCE_MQTT,       /* MQTT远程命令 */
    APP_COMMAND_SOURCE_SYSTEM,     /* 系统内部命令 */

    APP_COMMAND_SOURCE_COUNT
}AppCommandSource_t;


//完整app命令
typedef struct
{
    AppCommandType_t type;         // 命令类型 
    AppCommandSource_t source;     // 命令来源 
    uint32_t timestamp_ms;         // 命令产生时间
} AppCommand_t;
 
// 获取被stop命令主动取消的普通命令数量
//初始化
void AppCommand_Init(void);

//更新本地按键命令
void AppCommand_Update(void);

//向命令队列发送命令
bool AppCommand_Post(
    AppCommandType_t type,
    AppCommandSource_t source,
    uint32_t timestamp_ms
);

//向命令队列获取命令
AppCommand_t AppCommand_Get(AppCommand_t *command);

//判断是否有待处理的命令
bool AppCommand_HasPending(void);

//获取当前等待处理的命令的数量
uint8_t AppCommand_GetPendingCount(void);

//获取队满而丢失的普通命令数量
uint32_t AppCommand_GetLostCount(void);

//获取被STOP命令主动取消的普通命令数量。
uint32_t AppCommand_GetCancelledCount(void);

#endif