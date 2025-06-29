#include "tm1628a.h"
#include "bsp_adc.h"
#include "log.h"
#include "param.h"
#include "inout.h"
#include "motor.h"

#define DEBOUNCE_TIME_MS     50   // Debounce time to avoid false triggers
#define LONG_PRESS_TIME_MS  200   // Time to trigger continuous press
#define HEX_TO_BCD(val)   ((((val) / 10) << 4) + ((val) % 10))

#define MENU_KEY        1
#define UP_KEY          8
#define DOWN_KEY        2
#define ENTER_KEY       16


static uint8_t code_table[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, \
    0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71}; //# 0-F

typedef struct {
    uint8_t main_index; // 主菜单索引 (0~4)
    uint8_t sub_index[5];   // 每个主菜单下的子菜单索引
    uint8_t value_index;          // 三级菜单索引
    uint8_t level;      // 当前菜单层级（0~1）
} menu_state_t;

typedef struct 
{
    uint8_t current_key;
    uint8_t last_key;
    uint8_t is_key_pressed;
    uint32_t key_pressed_time;
    uint32_t current_time;
}key_t;

static key_t g_key = {0, 0, 0, 0, 0};

static menu_state_t g_menu_state = {0};  // 初始在第一级菜单第0项

static uint8_t g_level_refresh = 0;
static uint8_t g_errcode_display = 0;


// 第一级菜单项数
#define MAIN_MAX     5

// 第二级菜单项数（每个一级菜单对应不同数量的二级菜单项）
const uint8_t sub_menu_items[MAIN_MAX] = {2, 11, 8, 7, 2};

// 第三级菜单项数（每个二级菜单下都固定有100个三级菜单项）
// 三级菜单项数查找表（按实际需求填写）
const uint8_t value_menu_items[MAIN_MAX][12] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100}, // 第一级菜单项 0 的每个二级菜单对应的三级菜单项数
    {51, 31, 7, 61, 50, 14, 100, 2, 10, 10, 10, 10},   // 第一级菜单项 1
    {51, 2, 3, 14, 3, 2, 16, 5, 5, 5, 5, 5},               // 第一级菜单项 2
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}                // 第一级菜单项 3
};

static void menu_display(void)
{
    if((g_menu_state.level == 0) && (g_menu_state.main_index == 0))
        return ;
    switch(g_menu_state.level)
    {
        case 0:{/*第0级菜单*/
            uint8_t index = g_menu_state.main_index - 1;
            uint8_t data[8] = {0x00, 0x00, 0x73, 0x00,0x00, 0x00,0x00, 0x00};
            data[0] = code_table[index];
            data[4] = code_table[g_menu_state.sub_index[index] >> 4];
            data[6] = code_table[g_menu_state.sub_index[index] & 0x0f];
            tm1628a_write_continuous(data , sizeof(data)); 
        }break;
        case 1:{/*第1级菜单*/
            uint8_t data[8] = {0x5e, 0x00, 0x00, 0x00 , 0x00, 0x00,0x00, 0x00};
            if(g_menu_state.value_index < 100){
                uint8_t value = HEX_TO_BCD(g_menu_state.value_index);
                data[2] = code_table[value >> 4];
                data[4] = code_table[value & 0x0f];                
            }
            else{
                data[2] = 0x71;
                data[4] = 0x71;
            }
            tm1628a_write_continuous(data , sizeof(data));            
        }break;
        default:
            break;
    }
}


/**
 * @brief 控制菜单导航
 * 
 * @param key 按键值：
 *            1: 后退键
 *            8: 上键
 *            2: 下键
 *           16: 确认键
 */
void menu_ctl_func(uint8_t key)
{
    uint8_t write_protect = 0;

    // 仅在 level == 1 且不是访问“写保护参数”自身时才获取写保护状态
    if (g_menu_state.level == 1 &&
        !(g_menu_state.main_index == 4 && g_menu_state.sub_index[4] == 0)) {
        param_get(PARAM0X04, PARAM_WRITE_PROTECT, &write_protect);
    }
    g_errcode_display = 0; /*清除code显示，进入菜单显示*/
    switch (key)
    {
        case MENU_KEY: // 菜单键：在主菜单之间循环
            if (g_menu_state.level == 1) {
                // 返回上一级菜单（level 1 -> level 0）
                g_menu_state.level = 0;
            } else if (g_menu_state.level == 0) {
                // 在主菜单之间向下循环
                g_menu_state.sub_index[g_menu_state.main_index] = 0;
                g_menu_state.main_index = (g_menu_state.main_index + 1) % MAIN_MAX;
            }
            break;

        case UP_KEY: // 上键
        case DOWN_KEY: // 下键
            if (g_menu_state.level == 0) {
                // level == 0：在 sub_menu_items 中循环选择子菜单项
                uint8_t max_sub = sub_menu_items[g_menu_state.main_index];
                if (max_sub > 0) {
                    if (key == UP_KEY) { // 上键
                        g_menu_state.sub_index[g_menu_state.main_index] =
                            (g_menu_state.sub_index[g_menu_state.main_index] + max_sub - 1) % max_sub;
                    } else if (key == DOWN_KEY) { // 下键
                        g_menu_state.sub_index[g_menu_state.main_index] =
                            (g_menu_state.sub_index[g_menu_state.main_index] + 1) % max_sub;
                    }
                }
            }
            else if (g_menu_state.level == 1 && !write_protect) {
                // level == 1 且无写保护：在 value_menu_items 中循环选择参数值
                uint8_t current_sub_index = g_menu_state.sub_index[g_menu_state.main_index];
                uint8_t max_value = value_menu_items[g_menu_state.main_index][current_sub_index];

                if (max_value > 0) {
                    if (key == UP_KEY && g_menu_state.value_index > 0) {
                        g_menu_state.value_index--;
                    } else if (key == DOWN_KEY && g_menu_state.value_index < max_value - 1) {
                        g_menu_state.value_index++;
                    }
                }
            }
            break;

        case ENTER_KEY: // 确认键：进入下一级菜单或保存参数
            if (g_menu_state.level == 0) {
                if(g_menu_state.main_index == 0)
                    return ;
                g_menu_state.level++;
                uint8_t index = g_menu_state.main_index - 1;
                uint8_t value = 0;
                param_get((ModuleParameterType)index, g_menu_state.sub_index[index], &value);
                g_menu_state.value_index = value;
                
            } else {
                // 已在第一级菜单，按确认键保存参数后返回上一级
                if((g_menu_state.main_index == 4) && 
                    (g_menu_state.sub_index[4] == 1)  &&
                    (g_menu_state.value_index == 1)){
                    /*恢复默认参数*/
                    param_default();
                }
                else{
                    /*设置参数*/
                    uint8_t index = g_menu_state.main_index - 1;
                    param_set((ModuleParameterType)index, g_menu_state.sub_index[index], g_menu_state.value_index);
                }

                param_save();
                g_menu_state.level = 0;
            }
            break;
        default:
            // 其他按键不处理
            break;
    }
    menu_display();
}

static void show_speed_blink(void)
{
    static uint8_t blink_flag = 1;
    if(blink_flag)
    {
        uint8_t sp , value;
        inout_get_current_sp(&sp , &value);
        uint8_t data[8] = {0x00, 0x00, 0x00, 0x00,0x00, 0x00 ,0x00, 0x00};
        sp = HEX_TO_BCD(sp);
        value = HEX_TO_BCD(value);
        data[0] = code_table[sp & 0x0f];
        data[2] = 0x80;
        data[4] = code_table[value >> 4];
        data[6] = code_table[value & 0x0f];
        tm1628a_write_continuous(data , sizeof(data));  
        blink_flag = 0;       
    }
    else
    {
        uint8_t data[8] = {0x00, 0x00, 0x00, 0x00,0x00, 0x00 ,0x00, 0x00};
        tm1628a_write_continuous(data , sizeof(data));  
        blink_flag = 1;       
    }   
}

static void show_speed_info(void)
{ 
    static uint8_t sp_last;
    static uint32_t time_last;
    static uint8_t immi_flag = 1;

    if(motor_is_working())
    {
        /*电机运行中*/
        uint8_t sp , value;
        inout_get_current_sp(&sp , &value);
        if((sp != sp_last) || (g_level_refresh) || (immi_flag))
        {
            immi_flag = 0;
            uint8_t data[8] = {0x00, 0x00, 0x00, 0x00,0x00, 0x00 ,0x00, 0x00};
            sp = HEX_TO_BCD(sp);
            value = HEX_TO_BCD(value);
            data[0] = code_table[sp & 0x0f];
            data[2] = 0x80;
            data[4] = code_table[value >> 4];
            data[6] = code_table[value & 0x0f];
            tm1628a_write_continuous(data , sizeof(data));
 
            sp_last = sp;
            g_level_refresh = 0;
        }          
    }
    else
    {
        /*电机未运行*/
        immi_flag = 1;
        uint32_t now = HAL_GetTick();
        if((now - time_last > 500) || (g_level_refresh))
        {
            show_speed_blink();
            time_last = now;
            g_level_refresh = 0;
        }
    }
}


static void show_led_info(void)
{ 
    static uint8_t led_last;
    uint8_t led = 0;
    led |= inout_get_wire_value();          //LED1,断丝信号
    led |= inout_get_work_end() << 1;       //LED2,加工结束信号
    led |= inout_get_exceed_value() << 2;   //LED3,超程信号
    led |= motor_is_open_freq() << 3;       //LED4,开高频信号
    led |= inout_get_errcode() << 4;        //LED5,异常信号
    led |= 0 << 5;                          //LED6,掉电信号
    led |= inout_get_limit_value() << 6;    //LED7,限位信号
    if(led != led_last)
    {
        led_last = led;
        tm1628a_write_once(0xC8 , led);
    }
}


static void show_voltage_info(void)
{ 
    static uint32_t display_time;
    uint32_t now = HAL_GetTick();
    if(now - display_time < 200)
        return ;
    display_time = now;
    uint16_t voltage = bsp_get_voltage();
    if(voltage > 999)
        return ;

    uint8_t data[8] = {0x3E, 0x00, 0x00, 0x00,0x00, 0x00 ,0x00, 0x00};
    data[2] = code_table[voltage / 100] ;
    data[4] = code_table[(voltage / 10) % 10];
    data[6] = code_table[voltage % 10];
    tm1628a_write_continuous(data , sizeof(data)); 
    
}

/*显示速度和led灯信息*/
static void show_level0_and_led_info(void)
{
    show_led_info();
    if(g_errcode_display == 0) /*不显示erroce时才显示速度等信息*/
    {
        if(g_menu_state.level != 0)
            return ;
        if(g_menu_state.main_index != 0)
            return ;
        if(g_menu_state.sub_index[0] == 0){
            show_speed_info();
        }else if(g_menu_state.sub_index[0] == 1)
            show_voltage_info();
    }
}

static void scan_key(void)
{
    uint8_t key[5] = {0};
    tm1628a_read_key(key);
    g_key.current_key = key[4];
    g_key.current_time = HAL_GetTick();
    if (g_key.current_key != 0)
    {
        if (!g_key.is_key_pressed)
        {
            // First detection of key press
            g_key.key_pressed_time = g_key.current_time;
            g_key.is_key_pressed = 1;
            g_key.last_key = g_key.current_key;
            //logdbg("Key pressed: %d\n", g_key.current_key);
            menu_ctl_func(g_key.current_key);           
        }
        else if (g_key.current_key == g_key.last_key)
        {
            // Check for long press (every 200ms)
            if ((g_key.current_time - g_key.key_pressed_time) >= LONG_PRESS_TIME_MS)
            {
                uint32_t elapsed = g_key.current_time - g_key.key_pressed_time;
                // Trigger repeat every 200ms
                if ((elapsed % LONG_PRESS_TIME_MS) < DEBOUNCE_TIME_MS)
                {
                    //logdbg("Key repeating: %d\n", current_key);
                    menu_ctl_func(g_key.current_key);
                }
            }
        }
        else
        {
            // Different key pressed, reset state
            g_key.last_key = g_key.current_key;
            g_key.key_pressed_time = g_key.current_time;
            //logdbg("New key pressed: %d\n", current_key);
            menu_ctl_func(g_key.current_key);
        }
    }
    else
    {
        // Key released
        if (g_key.is_key_pressed)
        {
            //logdbg("Key released: %d\n", last_key);
            g_key.is_key_pressed = 0;
        }
    }
}

void hmi_init(void)
{
    tm1628a_init();
    menu_display();
}

void hmi_scan_key(void)
{
    scan_key();
    show_level0_and_led_info();
}

void hmi_stop_code(int code)
{
    logdbg("motor stop code: %d\n" , code);
    if(code > 999)
        return ;
    uint8_t data[8] = {0x79, 0x00, 0x00, 0x00,0x00, 0x00 ,0x00, 0x00};
    data[2] = code_table[code / 100] ;
    data[4] = code_table[(code / 10) % 10];
    data[6] = code_table[code % 10];
    tm1628a_write_continuous(data , sizeof(data)); 
    g_errcode_display = 1; 
}

void hmi_clear_menu(void)
{
    g_errcode_display = 0;
    g_menu_state.level = 0;
    g_menu_state.main_index = 0;
    g_menu_state.sub_index[0] = 0;
    g_menu_state.sub_index[1] = 0;
    g_menu_state.sub_index[2] = 0;
    g_menu_state.sub_index[3] = 0;
    g_menu_state.sub_index[4] = 0;
}