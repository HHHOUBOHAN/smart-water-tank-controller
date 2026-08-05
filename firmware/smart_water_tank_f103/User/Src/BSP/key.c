#include "BSP/key.h"
#include "BSP/board.h"

typedef struct
{
    GPIO_TypeDef *gpio_port;
    uint16_t gpio_pin;
    GPIO_PinState active_level;//按下的物理电平
} Key_Hardware_t;



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


//判断是否按下
static bool Key_ReadPressed(Key_Id_t key)
{

}


//按键配置
typedef struct
{
    uint16_t debounce_ms;//消抖确认时间

    uint16_t long_press_ms;//长按触发时间
    bool long_press_enabled;//是否启用长按

    uint16_t repeat_delay_ms;//按住多久开始连按
    uint16_t repeat_interval_ms;//连按事件间隔
    bool repeat_enabled;//是否启动连按

}Key_Config_t;

//四个按键配置
static const Key_Config_t key_config_table[KEY_ID_COUNT] =
{
    [KEY_ID_MODE] =
    {
        .debounce_ms        = 20,
        .long_press_ms      = 2000,
        .long_press_enabled = true,
        .repeat_enabled     = false
    },

    [KEY_ID_START] =
    {
        .debounce_ms        = 20,
        .long_press_enabled = false,
        .repeat_delay_ms    = 500,
        .repeat_interval_ms = 150,
        .repeat_enabled     = true
    },

    [KEY_ID_STOP] =
    {
        .debounce_ms        = 20,
        .long_press_enabled = false,
        .repeat_delay_ms    = 500,
        .repeat_interval_ms = 150,
        .repeat_enabled     = true
    },

    [KEY_ID_MUTE] =
    {
        .debounce_ms        = 20,
        .long_press_ms      = 2000,
        .long_press_enabled = true,
        .repeat_enabled     = false
    }
};
 

//按键运行状态

typedef struct
{
    bool raw_pressed;
    bool stable_pressed;

    bool suppress_until_release;
    bool long_press_reported;
    bool repeat_started;

    uint32_t raw_changed_at_ms;
    uint32_t pressed_at_ms;
    uint32_t next_repeat_at_ms;
} Key_Runtime_t;


//时间队列



//事件写入函数



//初始化


//状态机



//读取按键



//是否按下



//判断是否丢失


//