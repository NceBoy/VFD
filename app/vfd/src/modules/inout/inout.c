#include "main.h"
#include "bsp_adc.h"
#include "bsp_io.h"
#include "inout.h"
#include "service_motor.h"
#include "service_hmi.h"
#include "param.h"
#include "motor.h"

#define ACTIVE_NULL             (0xffffffff)
#define ACTIVE_LOW              (0)
#define ACTIVE_HIGH             (1)

#define ERROR_LEFT_KEY                      0x01            //左限位长时间触发
#define ERROR_RIGHT_KEY                     0x02            //右限位长时间触发
#define ERROR_DOUBLE_KEY                    0x04            //左右限位同时触发
#define ERROR_EXCEED_KEY                    0x08            //超程触发


typedef enum
{
    IO_ID_CTRL_MODE = 0x00,                     //点动or四键控制
    IO_ID_EXCEED_POLARITY = 0x01,              //超程极性
    IO_ID_END_POLARITY = 0x02,                 //加工结束极性
    IO_ID_LR_POLARITY = 0x03,                  //左右限位极性

    IO_ID_SP0 = 0x04,                          //速度调节0
    IO_ID_SP1 = 0x05,                          //速度调节1
    IO_ID_SP2 = 0x06,                          //速度调节2
    IO_ID_DEBUG = 0x07,                        //调试模式

    IO_ID_WIRE = 0x08,                         //断丝

    IO_ID_LIMIT_EXCEED = 0x09,                 //超程限位
    IO_ID_END = 10,                          //加工结束
    IO_ID_LIMIT_LEFT = 11,                   //左限位
    IO_ID_LIMIT_RIGHT = 12,                  //右限位
    
    

    IO_ID_MOTOR_START = 13,                  //开运丝
    IO_ID_MOTOR_STOP = 14,                   //关运丝
    IO_ID_PUMP_START = 15,                   //开水泵
    IO_ID_PUMP_STOP = 16,                    //关水泵

    IO_ID_IPM_VFO = 17,                      //IPM故障

    IO_ID_MAX,
}io_id_t;

typedef enum
{
    CTRL_MODE_JOG,          /*点动控制*/
    CTRL_MODE_FOUR_KEY,     /*四建控制*/
}ctl_mode_t;


typedef struct
{
    io_id_t         io_id;
    GPIO_TypeDef    *port;
    uint32_t        pin;
    uint32_t        now_ticks;
    uint32_t        debounce_ticks;
    uint32_t        exceed_ticks;
    uint32_t        active_polarity;
}vfd_io_t;



typedef struct 
{
    ctl_mode_t      ctl;
    uint8_t         flag[IO_ID_MAX];
    uint8_t         end;                /*工作结束信号触发*/
    uint8_t         sp;
    uint8_t         sp_manual;
    uint8_t         err;
}vfd_ctrl_t;

static vfd_ctrl_t g_vfd_ctrl;

static vfd_io_t g_vfd_io_tab[IO_ID_MAX] = {

    {IO_ID_CTRL_MODE       ,GPIOB, GPIO_PIN_12 ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_NULL},//控制模式设置，点动or四键
    {IO_ID_EXCEED_POLARITY ,GPIOB, GPIO_PIN_13 ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_NULL},//超程极性控制
    {IO_ID_END_POLARITY    ,GPIOB, GPIO_PIN_14 ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_NULL},//加工结束极性
    {IO_ID_LR_POLARITY     ,GPIOB, GPIO_PIN_15 ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_NULL},//左右限位极性

    {IO_ID_SP0             ,GPIOD, GPIO_PIN_2  ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_LOW},
    {IO_ID_SP1             ,GPIOC, GPIO_PIN_12 ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_LOW},
    {IO_ID_SP2             ,GPIOC, GPIO_PIN_11 ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_LOW},
    {IO_ID_DEBUG           ,GPIOB, GPIO_PIN_7  ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_LOW},//调试，有效电平0

    {IO_ID_WIRE            ,GPIOC, GPIO_PIN_13 ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_HIGH}, //断丝检测，按照常闭极性控制

    {IO_ID_LIMIT_EXCEED    ,GPIOA, GPIO_PIN_15 ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_LOW}, //超程
    {IO_ID_END             ,GPIOC, GPIO_PIN_10 ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_LOW}, //加工结束
    {IO_ID_LIMIT_LEFT      ,GPIOA, GPIO_PIN_11 ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_LOW}, //左限位
    {IO_ID_LIMIT_RIGHT     ,GPIOA, GPIO_PIN_12 ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_LOW}, //右限位

    {IO_ID_MOTOR_START     ,GPIOB, GPIO_PIN_3  ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_LOW},//开丝，有效电平0
    {IO_ID_MOTOR_STOP      ,GPIOB, GPIO_PIN_4  ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_LOW},//关丝，有效电平1
    {IO_ID_PUMP_START      ,GPIOB, GPIO_PIN_5  ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_LOW},//开水，有效电平0
    {IO_ID_PUMP_STOP       ,GPIOB, GPIO_PIN_6  ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_LOW},//关水，有效电平1

    {IO_ID_IPM_VFO         ,GPIOC, GPIO_PIN_7  ,0 , 50 , IO_TIMEOUT_MS ,ACTIVE_LOW},//关水，有效电平1
};

/*外部IO的错误信息处理*/
static void update_err(void)
{  
    /*断丝错误，由断丝逻辑单独处理，此处不再处理*/

    /*超程错误，由超程逻辑单独处理，此处不再处理*/

    /*左右限位长时间触发的错误，此处不再处理*/

    /*左右限位同时触发的错误，此处不再处理*/

    /*IPM模块的错误，此处不再处理*/

}


void motor_start_ctl(void)
{
    if((g_vfd_ctrl.flag[IO_ID_WIRE] != 0) &&(g_vfd_ctrl.flag[IO_ID_DEBUG] != 1))
    {
        ext_notify_stop_code(CODE_WIRE_BREAK);
        return ;
    }

    if(g_vfd_ctrl.flag[IO_ID_IPM_VFO] != 0)
    {
        ext_notify_stop_code(CODE_IPM);
        return ;
    }

    uint8_t speed = 0;
    uint8_t left = 0 , right = 0;
    if(g_vfd_ctrl.flag[IO_ID_SP0] != 0) /*手控盒控速*/
    {
        if(g_vfd_ctrl.sp_manual > 10 || g_vfd_ctrl.sp_manual < 8)
            return ;
        param_get(PARAM0X01, g_vfd_ctrl.sp_manual, &speed);
    }
    else 
    {
        if(g_vfd_ctrl.sp > 7)
            return ;
        param_get(PARAM0X01, g_vfd_ctrl.sp, &speed);
    }
        
    if(HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_LIMIT_LEFT].port, g_vfd_io_tab[IO_ID_LIMIT_LEFT].pin) == \
                                                    g_vfd_io_tab[IO_ID_LIMIT_LEFT].active_polarity)
        left = 1;
    if(HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_LIMIT_RIGHT].port, g_vfd_io_tab[IO_ID_LIMIT_RIGHT].pin) == \
                                                    g_vfd_io_tab[IO_ID_LIMIT_RIGHT].active_polarity)
        right = 1;
    uint8_t limit = left << 4 | right;
    switch(limit)
    {
        case 0x00 : {
            uint8_t dir = 0;
            param_get(PARAM0X03, PARAM_START_DIRECTION, &dir);
            if(dir == 0) dir = motor_target_current_dir(); /*之前的方向*/
            else if(dir == 1) dir = 0;      /*正向*/
            else if(dir == 2) dir = 1;      /*反向*/
            else dir  = 0;
            ext_motor_start(dir , speed);
        }break;
        case 0x01 : {
            g_vfd_ctrl.flag[IO_ID_LIMIT_RIGHT] = 1;
            ext_motor_start(0,speed);
        }break;
        case 0x10 : {
            g_vfd_ctrl.flag[IO_ID_LIMIT_LEFT] = 1;
            ext_motor_start(1,speed);
        }break;
        default:{
            ext_notify_stop_code(CODE_LIMIT_DOUBLE);
        }break;
    }
}

void motor_stop_ctl(stopcode_t code)
{
    ext_motor_break();
    ext_notify_stop_code((unsigned char)code);
    EXT_PUMP_DISABLE;
}

static void io_scan_active_polarity(void)
{
    /*极性控制只能在电机未启动时设置*/
    if(motor_is_working())
        return ;
    
    /*点动控制还是4键控制*/
    g_vfd_ctrl.ctl = (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_CTRL_MODE].port, g_vfd_io_tab[IO_ID_CTRL_MODE].pin) == GPIO_PIN_SET) ? \
                    CTRL_MODE_FOUR_KEY : CTRL_MODE_JOG ;

    /*超程信号极性*/
    g_vfd_io_tab[IO_ID_LIMIT_EXCEED].active_polarity = \
        (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_EXCEED_POLARITY].port, g_vfd_io_tab[IO_ID_EXCEED_POLARITY].pin) == GPIO_PIN_SET) ? \
        ACTIVE_LOW : ACTIVE_HIGH ;

    /*加工结束信号极性*/
    g_vfd_io_tab[IO_ID_END].active_polarity = \
        (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_END_POLARITY].port, g_vfd_io_tab[IO_ID_END_POLARITY].pin) == GPIO_PIN_SET) ? \
        ACTIVE_LOW : ACTIVE_HIGH ;

    /*左右信号极性*/
    if(HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_LR_POLARITY].port, g_vfd_io_tab[IO_ID_LR_POLARITY].pin) == GPIO_PIN_SET){
        g_vfd_io_tab[IO_ID_LIMIT_LEFT].active_polarity = ACTIVE_LOW;
        g_vfd_io_tab[IO_ID_LIMIT_RIGHT].active_polarity = ACTIVE_LOW;
    }       
    else{
        g_vfd_io_tab[IO_ID_LIMIT_LEFT].active_polarity = ACTIVE_HIGH;
        g_vfd_io_tab[IO_ID_LIMIT_RIGHT].active_polarity = ACTIVE_HIGH;        
    }
    /*断丝检测时间，消抖作用*/
    uint8_t value = 0;
    param_get(PARAM0X03, PARAM_WIRE_BREAK_TIME, &value); /*更新断丝检测时间*/
    g_vfd_io_tab[IO_ID_WIRE].debounce_ticks = value * 100; /*ms*/
}

#if 1
static void io_scan_debug(void)
{
    /*是否进入或者推出debug模式，debug模式一般是用来上丝*/
    static uint8_t debug_last = 1;
    uint8_t debug_now = (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_DEBUG].port, g_vfd_io_tab[IO_ID_DEBUG].pin) == GPIO_PIN_SET) ? 1 : 0;
    if(debug_now == debug_last)
        return ;
    debug_last = debug_now;
    if(debug_now == 0)
    {
        g_vfd_ctrl.flag[IO_ID_DEBUG] = 1;   /*进入debug模式*/
        //g_vfd_ctrl.flag[IO_ID_WIRE] = 0;   /*清除断丝错误显示*/
    }   
    else    /*退出debug模式*/
    {
        g_vfd_ctrl.flag[IO_ID_DEBUG] = 0;
        if(g_vfd_ctrl.flag[IO_ID_WIRE] && motor_is_working())
        {
            motor_stop_ctl(CODE_WIRE_BREAK);
        }
    }
}
#else
static void io_scan_debug(void)
{
    /*是否进入或者推出debug模式，debug模式一般是用来上丝*/
    g_vfd_ctrl.flag[IO_ID_DEBUG] = (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_DEBUG].port, g_vfd_io_tab[IO_ID_DEBUG].pin) == GPIO_PIN_SET) ? 0 : 1;
}
#endif


static void ctrl_speed(uint8_t sp)
{
    if(sp > 10)
        return ;
    uint8_t speed = 0;
    param_get(PARAM0X01, sp, &speed);
    ext_motor_speed(speed);  
}

static void io_ctrl_speed(void)
{
    if(g_vfd_ctrl.flag[IO_ID_SP0] != 0) /*用手控盒控制过速度*/
    {   /*通知手控盒取消相关显示*/

    }
    ctrl_speed(g_vfd_ctrl.sp);
}



static void io_scan_speed(void)
{  
    uint8_t sp0 = (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_SP0].port, g_vfd_io_tab[IO_ID_SP0].pin) == GPIO_PIN_SET) ? 1 : 0;
    uint8_t sp1 = (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_SP1].port, g_vfd_io_tab[IO_ID_SP1].pin) == GPIO_PIN_SET) ? 1 : 0;
    uint8_t sp2 = (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_SP2].port, g_vfd_io_tab[IO_ID_SP2].pin) == GPIO_PIN_SET) ? 1 : 0;

    uint8_t sp = (sp2 << 2) | (sp1 << 1) | sp0 ;
    if(sp == g_vfd_ctrl.sp)
        return ;

    g_vfd_ctrl.sp = sp;
    if(g_vfd_ctrl.end != 0)
        return ;
    if(motor_is_working())
    {
        io_ctrl_speed();
    }
    
}

static void io_ctrl_wire(void)
{
    if(g_vfd_ctrl.flag[IO_ID_DEBUG] != 0)
        return ;
    if(motor_is_working())
    {
        motor_stop_ctl(CODE_WIRE_BREAK);
    }
    
    return ;
}


static void io_scan_wire(void)
{  
    if(HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_WIRE].port, g_vfd_io_tab[IO_ID_WIRE].pin) == (GPIO_PinState)g_vfd_io_tab[IO_ID_WIRE].active_polarity)
    {
        if(g_vfd_io_tab[IO_ID_WIRE].now_ticks < IO_TIMEOUT_MS)
            g_vfd_io_tab[IO_ID_WIRE].now_ticks += IO_SCAN_INTERVAL;
    }
    else
    {
        g_vfd_ctrl.flag[IO_ID_WIRE] = 0;
        g_vfd_io_tab[IO_ID_WIRE].now_ticks = 0;
    }

    if(g_vfd_io_tab[IO_ID_WIRE].now_ticks > g_vfd_io_tab[IO_ID_WIRE].debounce_ticks)
    {
        if(g_vfd_ctrl.flag[IO_ID_WIRE] == 0)
        {
            g_vfd_ctrl.flag[IO_ID_WIRE] = 1;

            /*断丝*/
            io_ctrl_wire();
        }
    }    
}


static void io_ctrl_dir(void)
{
    if(g_vfd_ctrl.flag[IO_ID_END] != 0 ) /*加工结束*/
    {
        if(g_vfd_ctrl.flag[IO_ID_DEBUG] != 0)
            return ;
        uint8_t stop = 0;/*0:立即停止 1:靠右停 2:靠左停*/
        param_get(PARAM0X03, PARAM_STOP_MODE, &stop);
        EXT_PUMP_DISABLE;
        switch(stop)
        {
            case 1 :{
                if(g_vfd_ctrl.flag[IO_ID_LIMIT_RIGHT] != 0){
                    g_vfd_ctrl.end = 0;
                    motor_stop_ctl(CODE_END);
                }
                else
                {
                    //ctrl_speed(0);
                    g_vfd_ctrl.end = 1;
                    if(motor_target_current_dir() == 0){
                        ext_motor_reverse();
                    }
                }
            }break;
            case 2:{
                if(g_vfd_ctrl.flag[IO_ID_LIMIT_LEFT] != 0){
                    g_vfd_ctrl.end = 0;
                    motor_stop_ctl(CODE_END);
                }
                else
                {
                    //ctrl_speed(0);
                    g_vfd_ctrl.end = 1;
                    if(motor_target_current_dir() != 0){
                        ext_motor_reverse();
                    }
                }
            }break;
            default: {
                motor_stop_ctl(CODE_END);
            } break;
        }
    }

    if(g_vfd_ctrl.flag[IO_ID_LIMIT_EXCEED] != 0)
    {
        motor_stop_ctl(CODE_EXCEED);
        return ;
    }

    if(g_vfd_ctrl.flag[IO_ID_LIMIT_LEFT] != 0)
    {
        if(g_vfd_ctrl.end != 0)
        {
            motor_stop_ctl(CODE_END);
            g_vfd_ctrl.end = 0;
        }
        else 
        {
            if(motor_target_current_dir() == 0) /*正在向左运动*/
                ext_motor_reverse();
        }
    }
    if(g_vfd_ctrl.flag[IO_ID_LIMIT_RIGHT] != 0)
    {
        if(g_vfd_ctrl.end != 0)
        {
            motor_stop_ctl(CODE_END);
            g_vfd_ctrl.end = 0;
        }
        else
        {
            if(motor_target_current_dir() == 1) /*正在向右运动*/
                ext_motor_reverse();            
        }
    }
}

static void io_scan_direction(void)
{  
    if(motor_is_working() != 1)
    {
        g_vfd_io_tab[IO_ID_LIMIT_EXCEED].now_ticks = 0;
        g_vfd_io_tab[IO_ID_END].now_ticks = 0;
        g_vfd_io_tab[IO_ID_LIMIT_LEFT].now_ticks = 0;
        g_vfd_io_tab[IO_ID_LIMIT_RIGHT].now_ticks = 0;
        return ;
    }
        
    for(int i = IO_ID_LIMIT_EXCEED ; i <= IO_ID_LIMIT_RIGHT ; i++)
    {
        if(HAL_GPIO_ReadPin(g_vfd_io_tab[i].port, g_vfd_io_tab[i].pin) == (GPIO_PinState)g_vfd_io_tab[i].active_polarity)
        {
            if(g_vfd_io_tab[i].now_ticks < IO_TIMEOUT_MS)
                g_vfd_io_tab[i].now_ticks += IO_SCAN_INTERVAL;
        }
        else
        {
            g_vfd_ctrl.flag[i] = 0;
            g_vfd_io_tab[i].now_ticks = 0;
        }

        if(g_vfd_io_tab[i].now_ticks > g_vfd_io_tab[i].debounce_ticks)
        {
            if(g_vfd_ctrl.flag[i] == 0)
            {
                g_vfd_ctrl.flag[i] = 1;
                /*通知电机换向或者结束加工*/
                io_ctrl_dir();
            }
        }
    }
    //左限位长时间触发，异常
    if((g_vfd_io_tab[IO_ID_LIMIT_LEFT].now_ticks >= g_vfd_io_tab[IO_ID_LIMIT_LEFT].exceed_ticks))
    {
        g_vfd_ctrl.err |= ERROR_LEFT_KEY;
        if(motor_is_working())
        {
            motor_stop_ctl(CODE_LIMIT_TIMEOUT);
        }
    }
        
        
    //右限位长时间触发，异常
    if(g_vfd_io_tab[IO_ID_LIMIT_RIGHT].now_ticks >= g_vfd_io_tab[IO_ID_LIMIT_RIGHT].exceed_ticks)
    {
        g_vfd_ctrl.err |= ERROR_RIGHT_KEY;
        if(motor_is_working())
        {
            motor_stop_ctl(CODE_LIMIT_TIMEOUT);
        }
    } 
        

    /*左右限位同时触发*/
    if((g_vfd_ctrl.flag[IO_ID_LIMIT_LEFT] == 1) && (g_vfd_ctrl.flag[IO_ID_LIMIT_RIGHT] == 1))
    {
        g_vfd_ctrl.err |= ERROR_DOUBLE_KEY;
        if(motor_is_working())
        {
            motor_stop_ctl(CODE_LIMIT_DOUBLE);
        }
    }
        
    if(g_vfd_ctrl.flag[IO_ID_LIMIT_EXCEED] == 1)
        g_vfd_ctrl.err |= ERROR_EXCEED_KEY;
    else
        g_vfd_ctrl.err &= ~ERROR_EXCEED_KEY;

}



static void io_ctrl_onoff(void)
{
    switch(g_vfd_ctrl.ctl)
    {
        case CTRL_MODE_JOG :{ /*忽略关丝和关水逻辑*/
            if(g_vfd_ctrl.flag[IO_ID_MOTOR_START] != 0) /*开关丝*/
            {
                if(motor_is_working() != 1){
                    motor_start_ctl();
                }
                else{
                    motor_stop_ctl(CODE_END);
                }
            }
            if(g_vfd_ctrl.flag[IO_ID_PUMP_START] != 0) /*开关水*/
            {
                EXT_PUMP_TOGGLE;
            }            
        }break;
        case CTRL_MODE_FOUR_KEY :{
            if(g_vfd_ctrl.flag[IO_ID_MOTOR_START] != 0) /*开丝*/
            {
                if(motor_is_working() != 1){
                    motor_start_ctl();
                }
            }
            if(g_vfd_ctrl.flag[IO_ID_MOTOR_STOP] != 0) /*关丝*/
            {
                if(motor_is_working() == 1){
                    motor_stop_ctl(CODE_END);
                }
            }
            if(g_vfd_ctrl.flag[IO_ID_PUMP_START] != 0) /*开水*/
            {
                EXT_PUMP_ENABLE;
            }  
            if(g_vfd_ctrl.flag[IO_ID_PUMP_STOP] != 0) /*关水*/
            {
                EXT_PUMP_DISABLE;
            } 
        }break;
        default:break;
    }
}

static void io_scan_onoff(void)
{  
    for(int i = IO_ID_MOTOR_START ; i <= IO_ID_PUMP_STOP ; i++)
    {
        if(HAL_GPIO_ReadPin(g_vfd_io_tab[i].port, g_vfd_io_tab[i].pin) == g_vfd_io_tab[i].active_polarity)
        {
            if(g_vfd_io_tab[i].now_ticks < IO_TIMEOUT_MS)
                g_vfd_io_tab[i].now_ticks += IO_SCAN_INTERVAL;
        }
        else
        {
            g_vfd_io_tab[i].now_ticks = 0;
            g_vfd_ctrl.flag[i] = 0;
        }
            

        if(g_vfd_io_tab[i].now_ticks > g_vfd_io_tab[i].debounce_ticks)
        {
            if(g_vfd_ctrl.flag[i] == 0)
            {
                g_vfd_ctrl.flag[i] = 1;
                /*通知开或者关*/
                io_ctrl_onoff();
            }
        }       
    }
}

static void io_scan_ipm(void)
{
    if(HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_IPM_VFO].port, g_vfd_io_tab[IO_ID_IPM_VFO].pin) == g_vfd_io_tab[IO_ID_IPM_VFO].active_polarity)
    {
        if(g_vfd_io_tab[IO_ID_IPM_VFO].now_ticks < IO_TIMEOUT_MS)
            g_vfd_io_tab[IO_ID_IPM_VFO].now_ticks += IO_SCAN_INTERVAL;
    }
    else
    {
        g_vfd_io_tab[IO_ID_IPM_VFO].now_ticks = 0;
        g_vfd_ctrl.flag[IO_ID_IPM_VFO] = 0;
    }
        
    if(g_vfd_io_tab[IO_ID_IPM_VFO].now_ticks > g_vfd_io_tab[IO_ID_IPM_VFO].debounce_ticks)
    {
        if(g_vfd_ctrl.flag[IO_ID_IPM_VFO] == 0)
        {
            g_vfd_ctrl.flag[IO_ID_IPM_VFO] = 1;
            /*IPM故障*/
            if(motor_is_working()){
                motor_stop_ctl(CODE_END);
            }
        }
    }       
    
}

/*手控盒控速时，同步速度*/
void inout_sp_sync_from_ext(uint8_t sp)
{
    g_vfd_ctrl.sp_manual = sp;
    g_vfd_ctrl.flag[IO_ID_SP0] = 1;
    ctrl_speed(g_vfd_ctrl.sp_manual);
}

void inout_get_current_sp(unsigned char* sp , unsigned char* value)
{
    if(g_vfd_ctrl.flag[IO_ID_SP0])
    {
        *sp = g_vfd_ctrl.sp_manual;
        param_get(PARAM0X01, g_vfd_ctrl.sp_manual, value);
    }
    else
    {
        *sp = g_vfd_ctrl.sp;
        param_get(PARAM0X01, g_vfd_ctrl.sp, value);
    }

}

/*获取当前断丝状态*/
unsigned char inout_get_wire_value(void)
{
    return g_vfd_ctrl.flag[IO_ID_WIRE];
}

/*获取当前限位状态*/
unsigned char inout_get_limit_value(void)
{
    return g_vfd_ctrl.flag[IO_ID_LIMIT_LEFT] || g_vfd_ctrl.flag[IO_ID_LIMIT_RIGHT];
}

/*获取当前超程状态*/
unsigned char inout_get_exceed_value(void)
{
    return g_vfd_ctrl.flag[IO_ID_LIMIT_EXCEED];
}

/*获取当前errcode*/
unsigned char inout_get_errcode(void)
{
    return g_vfd_ctrl.err != 0 ? 1 : 0;
}

/*获取加工结束状态*/
unsigned char inout_get_work_end(void)
{
    return g_vfd_ctrl.flag[IO_ID_END];
}

void scan_voltage(void)
{
    int voltage = bsp_get_voltage();
}

void inout_init(void) 
{
    /*17个输入信号线*/
    bsp_io_init_input();
    /*3个输出信号线*/
    bsp_io_init_output();
    
    return ;
}

void inout_scan(void)
{
    /*step 1 . 极性*/
    io_scan_active_polarity();
    /*step 2 . 速度及调试模式*/
    io_scan_debug();
    io_scan_speed();
    /*step 3 . 断丝*/
    io_scan_wire();
    /*step 4 . 左右限位,超程和加工结束*/
    io_scan_direction();
    /*step 5 . 开关运丝和水泵*/
    io_scan_onoff();
    /*step 6 . 读取模块错误*/
    io_scan_ipm();
    /*step 7 . 电压检测*/
    scan_voltage();
    /*step 7 . 错误处理*/
    update_err();
}