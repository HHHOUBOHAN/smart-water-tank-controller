#ifndef KEY_H
#define KEY_H
#include <stdint.h>
#include <stdbool.h>


 

 


//按键名称和个数
typedef enum
{
    KEY_ID_MODE = 0,
    KEY_ID_START,
    KEY_ID_STOP,
    KEY_ID_MUTE,

    KEY_ID_COUNT
}Key_Id_t;


//按键事件
typedef enum
{
    KEY_EVENT_NONE = 0,      

    KEY_EVENT_DOWN,          
    KEY_EVENT_UP,           
    KEY_EVENT_SHORT_PRESS,   
    KEY_EVENT_LONG_PRESS,    
    KEY_EVENT_REPEAT        // 长按连续重复触发（长按连发）
}Key_EventType_t;


//按键事件机结构体
typedef struct
{
    Key_Id_t key;               // 发生事件的按键ID
    Key_EventType_t type;       // 事件类型
    uint32_t timestamp_ms;      // 事件发生时的系统时间戳(ms)
}Key_Event_t;


//初始化  
void Key_Init(uint32_t now_ms);

//按键扫描更新函数
void Key_Update(uint32_t now_ms);

//获取按键事件 
bool Key_GetEvent(Key_Event_t *event);

//判断是否按下  
bool Key_IsPressed(Key_Id_t key);


// 获得丢失事件 计数
uint32_t Key_GetLostEventCount(void);



#endif 

 

