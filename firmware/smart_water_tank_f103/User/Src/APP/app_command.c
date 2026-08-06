
#include "APP/app_command.h"
#include "BSP/key.h"
#include <stdint.h>
#include "stdbool.h"

#define APP_COMMAND_QUEUE_SIZE 16

static AppCommand_t app_command_queue[APP_COMMAND_QUEUE_SIZE];
static uint8_t app_command_head;
static uint8_t app_command_tail;
static uint8_t app_command_count;//有效命令数

static uint32_t app_command_lost_count;
static uint32_t app_command_cancelled_count;

//判断命令类型是否合法
static bool AppCommand_IsTypeValid(AppCommandType_t type)
{
    return
        ((uint32_t)type > (uint32_t)APP_COMMAND_NONE) &&
        ((uint32_t)type < (uint32_t)APP_COMMAND_COUNT);
}

//判断命令来源是否合法
static bool  AppCommand_IsSourceValid(AppCommandSource_t source)
{
    return
        ((uint32_t)source > (uint32_t)APP_COMMAND_SOURCE_KEY) &&
        ((uint32_t)source < (uint32_t)APP_COMMAND_SOURCE_COUNT);
}

//清空当前队列
static void AppCommand_ClearQueue(void)
{
    app_command_head = 0;
    app_command_tail = 0;
    app_command_count = 0;
}
 
 
//向队尾写入一条命令
static bool AppCommand_PushBack(
    AppCommandType_t type,
    AppCommandSource_t source,
    uint32_t timestamp_ms
)
{

    AppCommand_t *command;
    //

    //判断队列是否满
    if(app_command_count>APP_COMMAND_QUEUE_SIZE)
    {
        return false;
    }

    //获取位置 并进行赋值
    command=&app_command_queue[app_command_tail];
    command->source=source;
    command->timestamp_ms=timestamp_ms;
    command->type=type;

    app_command_tail++;
    
    if(app_command_tail>=APP_COMMAND_QUEUE_SIZE)
    {
        app_command_tail=0;
    }

    app_command_count++;
    return true;
}

//处理主页面的按键命令
static void AppCommand_HandleMainPageKey(
    const Key_Event_t *key_event
)
{
    if(key_event==NULL)
    {
        return;
    }
   
    switch(key_event->key)
    {
        //短按：自动/手动切换  长按：参数配置页面
        case KEY_ID_MODE:
        {
            if(key_event->type==KEY_EVENT_SHORT_PRESS)
            {
                //不考虑返回值 这个栈是16 一般不会栈溢出 以后如果扩展  就添加一个日志  
                (void)AppCommand_PushBack(
                    APP_COMMAND_MODE_TOGGLE,//模式切换
                    APP_COMMAND_SOURCE_KEY,
                    key_event->timestamp_ms
                );
            }
            else if(key_event->type==KEY_EVENT_LONG_PRESS)
            {
                (void)AppCommand_PushBack(
                    APP_COMMAND_ENTER_CONFIG,
                    APP_COMMAND_SOURCE_KEY,
                    key_event->timestamp_ms
                );
            }
            break;
        }
        //自动：主页面没有用途    手动： 主页面进行这个水泵的开启
        case KEY_ID_START:
        {
             
            //业务控制层（App_Run / AppControl 处理命令处）
            //拿到 APP_COMMAND_START 命令之后，读取当前运行模式，再差异化执行
            if(key_event->type==KEY_EVENT_SHORT_PRESS)
            {
                (void)AppCommand_PushBack(
                     APP_COMMAND_START, 
                    APP_COMMAND_SOURCE_KEY,
                    key_event->timestamp_ms
                );
            }
            break;
        }
        //
        case KEY_ID_STOP:
        {
            if(key_event->type==KEY_EVENT_SHORT_PRESS)
            {
                (void)AppCommand_PushBack(
                    APP_COMMAND_STOP, 
                    APP_COMMAND_SOURCE_KEY,
                    key_event->timestamp_ms
                );
            }
            break;
        }
        case KEY_ID_MUTE:
        {
            if(key_event->type==KEY_EVENT_SHORT_PRESS)
            {
                (void)AppCommand_PushBack(
                    APP_COMMAND_MUTE_TOGGLE, 
                    APP_COMMAND_SOURCE_KEY,
                    key_event->timestamp_ms
                );
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

//处理参数设置页面的按键事件。




//初始化app模块
void AppCommand_Init(void)
{

}

// 读取按键事件，并根据当前页面转换为APP命令。


//发送统一APP命令。

//从命令队列取出一条命令
