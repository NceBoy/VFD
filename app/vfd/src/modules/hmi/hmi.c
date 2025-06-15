#include "tm1628a.h"
#include "log.h"
#include "param.h"
#include "inout.h"
#include "motor.h"

#define DEBOUNCE_TIME_MS     50   // Debounce time to avoid false triggers
#define LONG_PRESS_TIME_MS  200   // Time to trigger continuous press
#define HEX_TO_BCD(val)   ((((val) / 10) << 4) + ((val) % 10))


static uint8_t code_table[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, \
    0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71}; //# 0-F

typedef struct {
    uint8_t level;      // 当前菜单层级（0~3）
    uint8_t index[4];   // 每一层的选中项索引（从0开始）
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

static menu_state_t g_menu_state = {0, {0, 0, 0,0}};  // 初始在第一级菜单第0项

static uint8_t g_level_refresh = 0;

// 第0级菜单项数
#define LEVEL0_MENU_ITEMS     1

// 第一级菜单项数
#define LEVEL1_MENU_ITEMS     4

// 第二级菜单项数（每个一级菜单对应不同数量的二级菜单项）
const uint8_t level2_menu_items[LEVEL1_MENU_ITEMS] = {11, 8, 7, 2};
//#define LEVEL2_MENU_ITEMS       12

// 第三级菜单项数（每个二级菜单下都固定有100个三级菜单项）
//#define LEVEL3_MENU_ITEMS    100
// 三级菜单项数查找表（按实际需求填写）
const uint8_t level3_menu_items[LEVEL1_MENU_ITEMS][12] = {
    {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100}, // 第一级菜单项 0 的每个二级菜单对应的三级菜单项数
    {51, 31, 7, 61, 50, 14, 100, 2, 10, 10, 10, 10},   // 第一级菜单项 1
    {51, 2, 3, 14, 3, 2, 16, 5, 5, 5, 5, 5},               // 第一级菜单项 2
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}                // 第一级菜单项 3
};

static void menu_display(void)
{
    switch(g_menu_state.level)
    {
        case 0:{/*第0级菜单*/
         
        }break;
        case 1:{/*第一级菜单*/
            uint8_t data[8] = {0x00, 0x00, 0x73, 0x00,0x00, 0x00,0x00, 0x00};
            data[0] = code_table[g_menu_state.index[1]];
            tm1628a_write_continuous(data , sizeof(data));            
        }break;
        case 2:{/*第二级菜单*/
            uint8_t data[8] = {0x00, 0x00, 0x73, 0x00 , 0x00, 0x00,0x00, 0x00};
            data[0] = code_table[g_menu_state.index[1]];
            data[4] = code_table[g_menu_state.index[2]];
            tm1628a_write_continuous(data , sizeof(data));            
        }break;
        case 3:{/*第三级菜单*/
            uint8_t data[8] = {0x5e, 0x00, 0x00, 0x00 , 0x00, 0x00,0x00, 0x00};
            if(g_menu_state.index[3] < 100){
                uint8_t value = HEX_TO_BCD(g_menu_state.index[3]);
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

#if 0
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
    // 如果处于三级菜单，并且不是访问写保护参数本身，则读取写保护标志
    if (g_menu_state.level == 3 &&
        !(g_menu_state.index[1] == 3 && g_menu_state.index[2] == 0)) {
        param_get(PARAM0X04, PARAM_WRITE_PROTECT, &write_protect);
    }
    
    switch (key)
    {
#if 1
        case 8: // 上键：选择上一项
            if (g_menu_state.level == 3 && write_protect) {
                break; // 写保护启用，禁止操作
            }
            if (g_menu_state.index[g_menu_state.level] > 0) {
                g_menu_state.index[g_menu_state.level]--;
            }
            break;

        case 2: // 下键：选择下一项
            if (g_menu_state.level == 3 && write_protect) {
                break; // 写保护启用，禁止操作
            }
            if (g_menu_state.level == 1) {
                if (g_menu_state.index[1] < LEVEL1_MENU_ITEMS - 1) {
                    g_menu_state.index[1]++;
                }
            } else if (g_menu_state.level == 2) {
                uint8_t items = level2_menu_items[g_menu_state.index[1]];
                if (g_menu_state.index[2] < items - 1) {
                    g_menu_state.index[2]++;
                }
            } else if (g_menu_state.level == 3) {
                uint8_t items = level3_menu_items[g_menu_state.index[1]][g_menu_state.index[2]];
                if (g_menu_state.index[3] < items - 1) {
                    g_menu_state.index[3]++;
                }
            }
            break;
#else
        case 8: // 上键：选择上一项（循环）
            if (g_menu_state.level == 3 && write_protect) {
                break; // 写保护启用，禁止操作
            }
            if (g_menu_state.level >= 1) {
                uint8_t items = 0;
                if (g_menu_state.level == 1) {
                    items = LEVEL1_MENU_ITEMS;
                }else if (g_menu_state.level == 2) {
                    items = level2_menu_items[g_menu_state.index[1]];
                } else if (g_menu_state.level == 3) {
                    items = level3_menu_items[g_menu_state.index[1]][g_menu_state.index[2]];
                }

                if (items > 0) {
                    g_menu_state.index[g_menu_state.level] =
                        (g_menu_state.index[g_menu_state.level] + items - 1) % items;
                }
            }
            break;

        case 2: // 下键：选择下一项（循环）
            if (g_menu_state.level == 3 && write_protect) {
                break; // 写保护启用，禁止操作
            }
            if (g_menu_state.level >= 1) {
                uint8_t items = 0;
                if (g_menu_state.level == 1) {
                    items = LEVEL1_MENU_ITEMS;
                }else if (g_menu_state.level == 2) {
                    items = level2_menu_items[g_menu_state.index[1]];
                } else if (g_menu_state.level == 3) {
                    items = level3_menu_items[g_menu_state.index[1]][g_menu_state.index[2]];
                }

                if (items > 0) {
                    g_menu_state.index[g_menu_state.level] =
                        (g_menu_state.index[g_menu_state.level] + 1) % items;
                }
            }
            break;


#endif

        case 16: // 确认键：进入下一级菜单或保存参数
            if (g_menu_state.level < 3) {
                g_menu_state.level++;
                if(g_menu_state.level == 3)
                {
                    uint8_t value = 0;
                    param_get((ModuleParameterType)g_menu_state.index[1], g_menu_state.index[2], &value);
                    g_menu_state.index[3] = value;
                }
            } else {
                // 已在第三级菜单，按确认键保存参数后返回上一级
                if((g_menu_state.index[1] == 3) && 
                    (g_menu_state.index[2] == 1)  &&
                    (g_menu_state.index[3] == 1))
                {
                    /*恢复默认参数*/
                    param_default();
                }
                else{
                    /*设置参数*/
                    param_set((ModuleParameterType)g_menu_state.index[1], g_menu_state.index[2], g_menu_state.index[3]);
                }

                param_save();
                g_menu_state.level--;
            }
            break;

        case 1: // 后退键：返回上一级菜单
            if (g_menu_state.level > 0) {
                g_menu_state.index[g_menu_state.level] = 0;
                g_menu_state.level--;
            }
            else if(g_menu_state.level == 0)
                g_menu_state.level++;
            break;

        default:
            // 其他按键不处理
            break;
    }
    menu_display();
}
#endif

#if 1
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
    // 如果处于三级菜单，并且不是访问写保护参数本身，则读取写保护标志
    if (g_menu_state.level == 3 &&
        !(g_menu_state.index[1] == 3 && g_menu_state.index[2] == 0)) {
        param_get(PARAM0X04, PARAM_WRITE_PROTECT, &write_protect);
    }
    
    switch (key)
    {
        case 8: // 上键：选择上一项（仅 level 1 & level 2 循环）
            if (g_menu_state.level == 3 && write_protect) {
                break; // 写保护启用，禁止操作
            }
            if (g_menu_state.level == 1 || g_menu_state.level == 2) {
                uint8_t items = 0;
                if (g_menu_state.level == 1) {
                    items = LEVEL1_MENU_ITEMS;
                } else if (g_menu_state.level == 2) {
                    items = level2_menu_items[g_menu_state.index[1]];
                }

                if (items > 0) {
                    g_menu_state.index[g_menu_state.level] =
                        (g_menu_state.index[g_menu_state.level] + items - 1) % items;
                }
            } else if (g_menu_state.level == 3) {
                if (g_menu_state.index[3] > 0) {
                    g_menu_state.index[3]--;
                }
            }
            break;

        case 2: // 下键：选择下一项（仅 level 1 & level 2 循环）
            if (g_menu_state.level == 3 && write_protect) {
                break; // 写保护启用，禁止操作
            }
            if (g_menu_state.level == 1 || g_menu_state.level == 2) {
                uint8_t items = 0;
                if (g_menu_state.level == 1) {
                    items = LEVEL1_MENU_ITEMS;
                } else if (g_menu_state.level == 2) {
                    items = level2_menu_items[g_menu_state.index[1]];
                }

                if (items > 0) {
                    g_menu_state.index[g_menu_state.level] =
                        (g_menu_state.index[g_menu_state.level] + 1) % items;
                }
            } else if (g_menu_state.level == 3) {
                uint8_t items = level3_menu_items[g_menu_state.index[1]][g_menu_state.index[2]];
                if (g_menu_state.index[3] < items - 1) {
                    g_menu_state.index[3]++;
                }
            }
            break;

        case 16: // 确认键：进入下一级菜单或保存参数
            if (g_menu_state.level < 3) {
                g_menu_state.level++;
                if(g_menu_state.level == 3){
                    uint8_t value = 0;
                    param_get((ModuleParameterType)g_menu_state.index[1], g_menu_state.index[2], &value);
                    g_menu_state.index[3] = value;
                }
            } else {
                // 已在第三级菜单，按确认键保存参数后返回上一级
                if((g_menu_state.index[1] == 3) && 
                    (g_menu_state.index[2] == 1)  &&
                    (g_menu_state.index[3] == 1)){
                    /*恢复默认参数*/
                    param_default();
                }
                else{
                    /*设置参数*/
                    param_set((ModuleParameterType)g_menu_state.index[1], g_menu_state.index[2], g_menu_state.index[3]);
                }

                param_save();
                g_menu_state.level--;
            }
            break;

        case 1: // 后退键：返回上一级菜单
            if (g_menu_state.level > 0) {
                g_menu_state.index[g_menu_state.level] = 0;
                g_menu_state.level--;
                if(g_menu_state.level == 0) g_level_refresh = 1;
            }
            else if(g_menu_state.level == 0)
                g_menu_state.level++;
            break;

        default:
            // 其他按键不处理
            break;
    }
    menu_display();
}
#endif
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
    if(g_menu_state.level == 0){
        if(motor_is_working()){
             /*电机运行中*/
            uint8_t sp , value;
            inout_get_current_sp(&sp , &value);
            if((sp != sp_last) || (g_level_refresh))
            {
                uint8_t data[8] = {0x00, 0x00, 0x73, 0x00,0x00, 0x00 ,0x00, 0x00};
                tm1628a_write_continuous(data , sizeof(data));  
                sp_last = sp;
                g_level_refresh = 0;
            }          
        }
        else{
            /*电机未运行*/
            uint32_t now = HAL_GetTick();
            if((now - time_last > 800) || (g_level_refresh))
            {
                show_speed_blink();
                time_last = now;
                g_level_refresh = 0;
            }
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

/*显示速度和led灯信息*/
static void show_speed_and_led_info(void)
{
    show_led_info();
    show_speed_info();
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
    show_speed_and_led_info();
}

void hmi_stop_code(int code)
{
    logdbg("motor stop code: %d\n" , code);
    if(code > 999)
        return ;
    uint8_t data[8] = {0x79, 0x00, 0x00, 0x00,0x00, 0x00 ,0x00, 0x00};
    data[2] = code / 100 ;
    data[4] = (code / 10) % 10;
    data[6] = code % 10;
    tm1628a_write_continuous(data , sizeof(data));  
}

void hmi_clear_menu(void)
{
    g_menu_state.level = 0;
    g_menu_state.index[0] = 0;
    g_menu_state.index[1] = 0;
    g_menu_state.index[2] = 0;
    g_menu_state.index[3] = 0;
}