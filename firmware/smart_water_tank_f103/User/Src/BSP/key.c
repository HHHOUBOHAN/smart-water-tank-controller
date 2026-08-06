#include "BSP/key.h"
#include "BSP/board.h"
#include <stdbool.h>
#include <stdint.h>


#include <stddef.h>
#include <stdint.h>
#include <string.h>
//按键事件环形缓冲区最大容量
#define KEY_EVENT_QUEUE_SIZE  16
//按键配置
typedef struct
{
    GPIO_TypeDef *gpio_port;    // GPIO端口基地址
    uint16_t gpio_pin;          // GPIO引脚号
    GPIO_PinState active_level; // 按键按下时的有效电平（高/低电平触发）
} Key_Hardware_t;


//按键功能参数配置
typedef struct
{
    uint16_t debounce_ms;          // 硬件消抖稳定时间(ms)
    uint16_t long_press_ms;        // 长按触发阈值时长(ms)
    bool long_press_enabled;       // 是否开启长按检测功能

    uint16_t repeat_delay_ms;      // 按住后延迟多久开始连发(ms)
    uint16_t repeat_interval_ms;  // 连发触发间隔(ms)
    bool repeat_enabled;           // 是否开启按住连续触发
}Key_Config_t;

//运行时状态结构体
typedef struct{
    bool raw_pressed;              // GPIO原始瞬时读取状态（未消抖）
    bool stable_pressed;           // 消抖完成后的稳定按键状态

    bool suppress_until_release;   // 上电屏蔽标志：上电按键按住时，必须松开才允许产生事件
    bool long_press_reported;      // 本次按压是否已经上报过长按事件（单次按压仅上报一次长按）
    bool repeat_started;           // 本次按压是否已经进入连续连发阶段

    uint32_t raw_changed_at_ms;     // GPIO原始电平发生跳变的时间戳
    uint32_t pressed_at_ms;         // 消抖确认按下的时间戳
    uint32_t next_repeat_at_ms;     // 下一次连发事件触发时间戳
}Key_Runtime_t;

//按键硬件映射表
static const Key_Hardware_t key_hardware_table[KEY_ID_COUNT] =
{
    [KEY_ID_MODE] =
    {
        .gpio_port    = BOARD_KEY_MODE_GPIO_PORT,
        .gpio_pin     = BOARD_KEY_MODE_GPIO_PIN,
        .active_level = BOARD_KEY_MODE_ACTIVE_LEVEL
    },

    [KEY_ID_START] =
    {
        .gpio_port    = BOARD_KEY_START_GPIO_PORT,
        .gpio_pin     = BOARD_KEY_START_GPIO_PIN,
        .active_level = BOARD_KEY_START_ACTIVE_LEVEL
    },

    [KEY_ID_STOP] =
    {
        .gpio_port    = BOARD_KEY_STOP_GPIO_PORT,
        .gpio_pin     = BOARD_KEY_STOP_GPIO_PIN,
        .active_level = BOARD_KEY_STOP_ACTIVE_LEVEL
    },

    [KEY_ID_MUTE] =
    {
        .gpio_port    = BOARD_KEY_MUTE_GPIO_PORT,
        .gpio_pin     = BOARD_KEY_MUTE_GPIO_PIN,
        .active_level = BOARD_KEY_MUTE_ACTIVE_LEVEL
    }
};


//各按键功能参数配置
static const Key_Config_t key_config_table[KEY_ID_COUNT] =
{
    /* MODE按键：短按切换自动/手动模式，长按2s进入参数设置页面，无连发 */
    [KEY_ID_MODE] =
    {
        .debounce_ms         = 20,        //消抖时间
        .long_press_ms       = 2000,      //长按阈值
        .long_press_enabled  = true,      //是否开启长按 
        .repeat_delay_ms     = 0,         // 连发启动延迟
        .repeat_interval_ms  = 0,         // 连发间隔
        .repeat_enabled      = false      //是否开启连发
    },

    /* START按键：短按启动水泵，设置页面长按连发加数值，按住500ms后每150ms触发一次 */
    [KEY_ID_START] =
    {
        .debounce_ms         = 20,
        .long_press_ms       = 0,
        .long_press_enabled  = false,
        .repeat_delay_ms     = 500,
        .repeat_interval_ms  = 150,
        .repeat_enabled      = true
    },

    /* STOP按键：短按停止水泵，设置页面长按连发减数值，按住500ms后每150ms触发一次 */
    [KEY_ID_STOP] =
    {
        .debounce_ms         = 20,
        .long_press_ms       = 0,
        .long_press_enabled  = false,
        .repeat_delay_ms     = 500,
        .repeat_interval_ms  = 150,
        .repeat_enabled      = true
    },

    /* MUTE消音按键：短按关闭蜂鸣器告警，长按2s放弃参数修改退出设置页，无连发 */
    [KEY_ID_MUTE] =
    {
        .debounce_ms         = 20,
        .long_press_ms       = 2000,
        .long_press_enabled  = true,
        .repeat_delay_ms     = 0,
        .repeat_interval_ms  = 0,
        .repeat_enabled      = false
    }
};

//四个按键实时运行状态
static Key_Runtime_t key_runtime_table[KEY_ID_COUNT];

//按键事件环形缓冲区数组
static Key_Event_t key_event_queue[KEY_EVENT_QUEUE_SIZE];

static uint8_t key_event_head;      // 环形队列头指针：读取事件位置
static uint8_t key_event_tail;      // 环形队列尾指针：写入事件位置
static uint8_t key_event_count;     // 当前队列有效事件总数

static uint32_t key_lost_event_count; // 队列溢出丢失事件统计，用于调试排查卡顿


//时间间隔判断函数
static bool Key_TimeElapsed(
    uint32_t now_ms,
    uint32_t start_ms,
    uint32_t duration_ms
)
{
    return ((uint32_t)(now_ms - start_ms) >= duration_ms);
}

//目标时间到达判断函数
static bool Key_TimeReached(
    uint32_t now_ms,
    uint32_t target_ms
)
{
    return ((int32_t)(now_ms - target_ms) >= 0);
}

//读取当前电平  true 按键按下；false 按键松开  通过对应的按键id来实现
static bool Key_ReadPressed(Key_Id_t key)
{
    GPIO_PinState current_level;

    if ((uint32_t)key >= (uint32_t)KEY_ID_COUNT)
    {
        return false;
    }

    current_level = HAL_GPIO_ReadPin(
        key_hardware_table[key].gpio_port,
        key_hardware_table[key].gpio_pin
    );

    return (current_level == key_hardware_table[key].active_level);
}

//将按键事件存入环形环形缓冲区
static void Key_PushEvent(
    Key_Id_t key,
    Key_EventType_t type,
    uint32_t now_ms
)   
{
    Key_Event_t *event;
    //没有读取到 直接返回
    if(type == KEY_EVENT_NONE)
    {
        return;
    }
    //队列如果满  直接丢弃 
    if(key_event_count>=KEY_EVENT_QUEUE_SIZE)
    {
        key_lost_event_count++;
        return;
    }

    //获取空闲位置
    event = &key_event_queue[key_event_tail];
    event->key=key;
    event->type=type;
    event->timestamp_ms=now_ms;

    key_event_tail++;

    if(key_event_tail>=KEY_EVENT_QUEUE_SIZE)
    {
        key_event_tail=0;
    }
    
    key_event_count++;
}




//初始化  
void Key_Init(uint32_t now_ms)
{
    uint32_t index;

    //清空按键运行状态与事件缓冲区
    memset(key_runtime_table, 0, sizeof(key_runtime_table));
    memset(key_event_queue, 0, sizeof(key_event_queue));

    key_event_head = 0;
    key_event_tail = 0;
    key_event_count = 0;
    key_lost_event_count = 0;

    
    for (index = 0U; index < (uint32_t)KEY_ID_COUNT; index++)
    {
        const bool pressed = Key_ReadPressed((Key_Id_t)index);//读取每个按键当前真实 GPIO 电平  判断开机瞬间按键有没有被按住
        Key_Runtime_t *runtime = &key_runtime_table[index];//拿到当前按键对应的运行状态结构体指针

        //原始电平=当前真实电平
        runtime->raw_pressed    = pressed;
        //消抖稳定电平直接同步为当前电平
        runtime->stable_pressed = pressed;

        // 上电按键处于按下状态，屏蔽所有事件直到松开一次
        runtime->suppress_until_release = pressed;

        //没有上报长按
        runtime->long_press_reported = false;
        //没有进行连发
        runtime->repeat_started      = false;

        //跳变时间差
        runtime->raw_changed_at_ms = now_ms;
        runtime->pressed_at_ms      = now_ms;

        runtime->next_repeat_at_ms = now_ms + key_config_table[index].repeat_delay_ms;
    }
}

//按键扫描更新函数
void Key_Update(uint32_t now_ms)
{
    //遍历按键  读取配置参数 获取按键实时运行状态   实时读取gpio原始电平



    //本次读取电平和上次读取电平不同  说明引脚松动或按下   更新最新原始电平 记录电平跳变时刻

    //

}

//获取按键事件 
bool Key_GetEvent(Key_Event_t *event)
{
    if((event==NULL)||(key_event_count==0))
    {
        return false;
    }
    *event=key_event_queue[key_event_head];
    key_event_head++;
    if(key_event_head>=KEY_EVENT_QUEUE_SIZE)
    {
        key_event_head=0;
    }
    key_event_count--;
    return true;
}

//判断是否按下  
bool Key_IsPressed(Key_Id_t key)
{
    if((uint32_t)key>= KEY_ID_COUNT)
    {
        return false;
    }
    return key_runtime_table[key].stable_pressed;
}


// 获得丢失事件 计数
uint32_t Key_GetLostEventCount(void)
{
    return key_lost_event_count;
}



 



















