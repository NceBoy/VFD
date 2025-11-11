#include "main.h"
#include "bsp_adc.h"
#include "bsp_io.h"
#include "inout.h"
#include "service_motor.h"
#include "service_hmi.h"
#include "service_data.h"
#include "param.h"
#include "motor.h"

#define ACTIVE_NULL             (0xffffffff)
#define ACTIVE_LOW              (0)
#define ACTIVE_HIGH             (1)

#define ERROR_LEFT_KEY                      0x01            //左限位长时间触发
#define ERROR_RIGHT_KEY                     0x02            //右限位长时间触发
#define ERROR_DOUBLE_KEY                    0x04            //左右限位同时触发
#define ERROR_EXCEED_KEY                    0x08            //超程触发

static int g_ipm_vfo_flag = 0;

typedef struct 
{
    uint16_t ctrl_delay;  
    uint8_t  real_status;
    uint8_t  ctrl_value;  
}pump_ctl_t;

typedef struct
{
    uint32_t value;
    uint32_t up_ticks;
    uint32_t down_ticks;
}speed_io_t;


typedef enum
{
    IO_ID_SP0           = 0x00,     //速度调节0
    IO_ID_SP1           = 0x01,     //速度调节1
    IO_ID_SP2           = 0x02,     //速度调节2
    IO_ID_DEBUG         = 0x03,     //调试模式

    IO_ID_WIRE          = 0x04,     //断丝

    IO_ID_LIMIT_EXCEED  = 0x05,     //超程限位
    IO_ID_END           = 0x06,     //加工结束
    IO_ID_LIMIT_LEFT    = 0x07,     //左限位
    IO_ID_LIMIT_RIGHT   = 0x08,     //右限位
    
    IO_ID_MOTOR_START   = 0x09,     //开运丝
    IO_ID_MOTOR_STOP    = 0x0A,     //关运丝
    IO_ID_PUMP_START    = 0x0B,     //开水泵
    IO_ID_PUMP_STOP     = 0x0C,     //关水泵

    IO_ID_MAX,
}io_id_t;

typedef enum
{
    CTRL_MODE_JOG = 0,          /*点动控制*/
    CTRL_MODE_FOUR_KEY,         /*四建控制*/
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

static uint8_t g_vfd_voltage_flag;

static vfd_ctrl_t g_vfd_ctrl;

static pump_ctl_t   g_pump_ctl;

static vfd_io_t g_vfd_io_tab[IO_ID_MAX] = {

    {IO_ID_SP0             ,GPIOD, GPIO_PIN_2  ,0 , 20 , IO_TIMEOUT_MS ,ACTIVE_LOW},
    {IO_ID_SP1             ,GPIOC, GPIO_PIN_12 ,0 , 20 , IO_TIMEOUT_MS ,ACTIVE_LOW},
    {IO_ID_SP2             ,GPIOC, GPIO_PIN_11 ,0 , 20 , IO_TIMEOUT_MS ,ACTIVE_LOW},
    {IO_ID_DEBUG           ,GPIOB, GPIO_PIN_7  ,0 , 20 , IO_TIMEOUT_MS ,ACTIVE_LOW},//调试，有效电平0

    {IO_ID_WIRE            ,GPIOC, GPIO_PIN_13 ,0 , 20 , IO_TIMEOUT_MS ,ACTIVE_HIGH}, //断丝检测，按照常闭极性控制

    {IO_ID_LIMIT_EXCEED    ,GPIOA, GPIO_PIN_15 ,0 , 20 , IO_TIMEOUT_MS ,ACTIVE_LOW}, //超程
    {IO_ID_END             ,GPIOC, GPIO_PIN_10 ,0 , 20 , IO_TIMEOUT_MS ,ACTIVE_LOW}, //加工结束
    {IO_ID_LIMIT_LEFT      ,GPIOA, GPIO_PIN_11 ,0 , 20 , IO_TIMEOUT_MS ,ACTIVE_LOW}, //左限位
    {IO_ID_LIMIT_RIGHT     ,GPIOA, GPIO_PIN_12 ,0 , 20 , IO_TIMEOUT_MS ,ACTIVE_LOW}, //右限位

    {IO_ID_MOTOR_START     ,GPIOB, GPIO_PIN_14  ,0 , 20 , IO_TIMEOUT_MS ,ACTIVE_LOW},//开丝，有效电平0
    {IO_ID_MOTOR_STOP      ,GPIOB, GPIO_PIN_15  ,0 , 20 , IO_TIMEOUT_MS ,ACTIVE_LOW},//关丝，有效电平1
    {IO_ID_PUMP_START      ,GPIOB, GPIO_PIN_5  ,0 , 20 , IO_TIMEOUT_MS ,ACTIVE_LOW},//开水，有效电平0
    {IO_ID_PUMP_STOP       ,GPIOB, GPIO_PIN_6  ,0 , 20 , IO_TIMEOUT_MS ,ACTIVE_LOW},//关水，有效电平1
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

int motor_mode_get(void)
{
    return g_vfd_ctrl.flag[IO_ID_DEBUG];
}


void motor_start_ctl(void)
{
    if(motor_is_working() == 1)
        return ;

    if((g_vfd_ctrl.flag[IO_ID_WIRE] != 0) &&(g_vfd_ctrl.flag[IO_ID_DEBUG] != 1))
    {
        ext_notify_stop_code(CODE_WIRE_BREAK);
        ext_send_report_to_data(0,CODE_WIRE_BREAK,0xFF,0xFF,0xFF,0xFF,motor_mode_get());
        return ;
    }

    /*电压错误，1:欠压,2:过压, 3:掉电*/
    if(g_vfd_voltage_flag != 0)
    {
        ext_notify_stop_code(g_vfd_voltage_flag + 4);
        ext_send_report_to_data(0,g_vfd_voltage_flag + 4,0xFF,0xFF,0xFF,0xFF,motor_mode_get());
        return ;
    }

    if(g_ipm_vfo_flag != 0)
    {
        ext_notify_stop_code(CODE_IPM_VFO);
        ext_send_report_to_data(0,CODE_IPM_VFO,0xFF,0xFF,0xFF,0xFF,motor_mode_get());
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
            ext_send_report_to_data(0,0xFF,speed,0xFF,0,dir,motor_mode_get());
        }break;
        case 0x01 : {
            g_vfd_ctrl.flag[IO_ID_LIMIT_RIGHT] = 1;
            ext_motor_start(0,speed);
            ext_send_report_to_data(0,0xFF,speed,0xFF,0,0,motor_mode_get());
        }break;
        case 0x10 : {
            g_vfd_ctrl.flag[IO_ID_LIMIT_LEFT] = 1;
            ext_motor_start(1,speed);
            ext_send_report_to_data(0,0xFF,speed,0xFF,0,1,motor_mode_get());
        }break;
        default:{
            ext_notify_stop_code(CODE_LIMIT_DOUBLE);
            ext_send_report_to_data(0,CODE_LIMIT_DOUBLE,0xFF,0xFF,0xFF,0xFF,motor_mode_get());
        }break;
    }
}


void motor_stop_ctl(stopcode_t code)
{
    if(motor_is_working() == 0)
        return ;
    
    ext_motor_brake();
    ext_notify_stop_code((unsigned char)code);
    ext_send_report_to_data(0,code,0xFF,0xFF,1,0xFF,motor_mode_get());
}

static void io_scan_active_polarity(void)
{
    /*极性控制只能在电机未启动时设置*/
    if(motor_is_working())
        return ;
    uint8_t value = 0;

    /*极性检测选择从参数读取而不是外部IO输入*/
    param_get(PARAM0X03, PARAM_JOY_KEY_SIGNAL, &value); /*点动还是4键控制*/
    g_vfd_ctrl.ctl = (ctl_mode_t)value;

    param_get(PARAM0X03, PARAM_EXCEED_SIGNAL, &value); /*超程信号极性,0:常开 1:常闭*/
    g_vfd_io_tab[IO_ID_LIMIT_EXCEED].active_polarity =  (value == 0 ? ACTIVE_LOW : ACTIVE_HIGH);    

    param_get(PARAM0X03, PARAM_LEFT_RIGHT_SIGNAL, &value); /*左右换向信号极性,0:常开 1:常闭*/
    g_vfd_io_tab[IO_ID_LIMIT_LEFT].active_polarity =  (value == 0 ? ACTIVE_LOW : ACTIVE_HIGH); 
    g_vfd_io_tab[IO_ID_LIMIT_RIGHT].active_polarity =  (value == 0 ? ACTIVE_LOW : ACTIVE_HIGH);

    param_get(PARAM0X03, PARAM_WORK_END_SIGNAL, &value); /*加工结束信号极性,0:常开 1:常闭*/
    g_vfd_io_tab[IO_ID_END].active_polarity =  (value == 0 ? ACTIVE_LOW : ACTIVE_HIGH);     

    /*断丝检测时间，消抖作用*/
    param_get(PARAM0X03, PARAM_WIRE_BREAK_TIME, &value); /*更新断丝检测时间*/
    g_vfd_io_tab[IO_ID_WIRE].debounce_ticks = value * 100; /*ms*/

}

void inout_mode_sync_from_ext(unsigned char mode)
{
    if(mode != 0) /*进入debug模式*/
    {
        g_vfd_ctrl.flag[IO_ID_DEBUG] = 1;   /*进入debug模式*/
    }
    else /*退出debug模式*/
    {
        g_vfd_ctrl.flag[IO_ID_DEBUG] = 0;
        if(g_vfd_ctrl.flag[IO_ID_WIRE] && motor_is_working())
        {
            motor_stop_ctl(CODE_WIRE_BREAK);
        }
    }
    ext_send_report_to_data(0,0xFF,0xFF,0xFF,0xFF,0xFF,motor_mode_get());
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
            pump_ctl_set_value(0 , 500);
        }
    }
    ext_send_report_to_data(0,0xFF,0xFF,0xFF,0xFF,0xFF,motor_mode_get());
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
    ext_send_report_to_data(0,0xFF,speed,0xFF,0xFF,0xFF,motor_mode_get());
}

static void io_ctrl_speed(void)
{
    if(g_vfd_ctrl.flag[IO_ID_SP0] != 0) /*用手控盒控制过速度*/
    {   /*通知手控盒取消相关显示*/
        g_vfd_ctrl.flag[IO_ID_SP0] = 0;
    }
    ctrl_speed(g_vfd_ctrl.sp);
}

/*手控盒控速时，同步速度*/
void inout_sp_sync_from_ext(uint8_t sp)
{
    g_vfd_ctrl.sp_manual = sp;
    g_vfd_ctrl.flag[IO_ID_SP0] = 1;
    ctrl_speed(g_vfd_ctrl.sp_manual);
}

void ext_ctl_pump(int period)
{
    if(g_pump_ctl.real_status == g_pump_ctl.ctrl_value)
        return ;
    
    if(g_pump_ctl.ctrl_delay > period)
        g_pump_ctl.ctrl_delay -= period;
    else
    {
        g_pump_ctl.ctrl_delay = 0;
    }
    if(g_pump_ctl.ctrl_delay == 0)
    {
        bsp_io_ctrl_pump(g_pump_ctl.ctrl_value);
        int value = g_pump_ctl.ctrl_value ? 0 : 1;
        ext_send_report_to_data(0,0,0xFF,value,0xFF,0xFF,motor_mode_get());
        g_pump_ctl.real_status = g_pump_ctl.ctrl_value;
    }
}

void pump_ctl_set_value(int value , int delay)
{
    if(value == g_pump_ctl.ctrl_value)
        return ;
    g_pump_ctl.ctrl_value = value;
    g_pump_ctl.ctrl_delay = delay;
    return ;
}

int pump_ctl_get_value(void)
{
    return g_pump_ctl.real_status;
}

int pump_ctl_toggle_value(void)
{
    int value = pump_ctl_get_value() ? 0 : 1;
    pump_ctl_set_value(value,0);
    return value;
}


static speed_io_t g_sp0 = {0};
static speed_io_t g_sp1 = {0};
static speed_io_t g_sp2 = {0};
static void io_scan_speed_debound(void)
{ 
    if(HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_SP0].port, g_vfd_io_tab[IO_ID_SP0].pin) == GPIO_PIN_SET)
    {
        g_sp0.down_ticks = 0;
        if(g_sp0.up_ticks < 200)
            g_sp0.up_ticks += MAIN_CTL_PERIOD;
        if(g_sp0.up_ticks >= 20)
            g_sp0.value = 1;
    }
    else{
        g_sp0.up_ticks = 0;
        if(g_sp0.down_ticks < 200)
            g_sp0.down_ticks += MAIN_CTL_PERIOD;
        if(g_sp0.down_ticks >= 20)
            g_sp0.value = 0;
    }

    if(HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_SP1].port, g_vfd_io_tab[IO_ID_SP1].pin) == GPIO_PIN_SET)
    {
        g_sp1.down_ticks = 0;
        if(g_sp1.up_ticks < 200)
            g_sp1.up_ticks += MAIN_CTL_PERIOD;
        if(g_sp1.up_ticks >= 20)
            g_sp1.value = 1;
    }
    else{
        g_sp1.up_ticks = 0;
        if(g_sp1.down_ticks < 200)
            g_sp1.down_ticks += MAIN_CTL_PERIOD;
        if(g_sp1.down_ticks >= 20)
            g_sp1.value = 0;
    }

    if(HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_SP2].port, g_vfd_io_tab[IO_ID_SP2].pin) == GPIO_PIN_SET)
    {
        g_sp2.down_ticks = 0;
        if(g_sp2.up_ticks < 200)
            g_sp2.up_ticks += MAIN_CTL_PERIOD;
        if(g_sp2.up_ticks >= 20)
            g_sp2.value = 1;
    }
    else{
        g_sp2.up_ticks = 0;
        if(g_sp2.down_ticks < 200)
            g_sp2.down_ticks += MAIN_CTL_PERIOD;
        if(g_sp2.down_ticks >= 20)
            g_sp2.value = 0;
    }
}

static void io_scan_speed(void)
{  
    //uint8_t sp0 = (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_SP0].port, g_vfd_io_tab[IO_ID_SP0].pin) == GPIO_PIN_SET) ? 1 : 0;
    //uint8_t sp1 = (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_SP1].port, g_vfd_io_tab[IO_ID_SP1].pin) == GPIO_PIN_SET) ? 1 : 0;
    //uint8_t sp2 = (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_SP2].port, g_vfd_io_tab[IO_ID_SP2].pin) == GPIO_PIN_SET) ? 1 : 0;
    io_scan_speed_debound();
    uint8_t sp = (g_sp2.value << 2) | (g_sp1.value << 1) | g_sp0.value ;
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
        pump_ctl_set_value(0 , 500);
    }
    
    return ;
}


static void io_scan_wire(void)
{  
    if(HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_WIRE].port, g_vfd_io_tab[IO_ID_WIRE].pin) == (GPIO_PinState)g_vfd_io_tab[IO_ID_WIRE].active_polarity)
    {
        if(g_vfd_io_tab[IO_ID_WIRE].now_ticks < IO_TIMEOUT_MS)
            g_vfd_io_tab[IO_ID_WIRE].now_ticks += MAIN_CTL_PERIOD;
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
        switch(stop)
        {
            case 1 :{
                if(g_vfd_ctrl.flag[IO_ID_LIMIT_RIGHT] != 0){
                    g_vfd_ctrl.end = 0;
                    motor_stop_ctl(CODE_END);
                    pump_ctl_set_value(0 , 500);
                }
                else
                {
                    //ctrl_speed(0);
                    g_vfd_ctrl.end = 1;
                    if(motor_target_current_dir() == 0){
                        ext_motor_reverse();
                        ext_send_report_to_data(0,0xFF,0xFF,0xFF,0xFF,1,motor_mode_get());
                    }
                }
            }break;
            case 2:{
                if(g_vfd_ctrl.flag[IO_ID_LIMIT_LEFT] != 0){
                    g_vfd_ctrl.end = 0;
                    motor_stop_ctl(CODE_END);
                    pump_ctl_set_value(0 , 500);
                }
                else
                {
                    //ctrl_speed(0);
                    g_vfd_ctrl.end = 1;
                    if(motor_target_current_dir() != 0){
                        ext_motor_reverse();
                        ext_send_report_to_data(0,0xFF,0xFF,0xFF,0xFF,0,motor_mode_get());
                    }
                }
            }break;
            default: {
                motor_stop_ctl(CODE_END);
                pump_ctl_set_value(0 , 500);
            } break;
        }
    }

    if(g_vfd_ctrl.flag[IO_ID_LIMIT_EXCEED] != 0) /*超程*/
    {
        motor_stop_ctl(CODE_EXCEED);
        pump_ctl_set_value(0 , 500);
        return ;
    }

    if(g_vfd_ctrl.flag[IO_ID_LIMIT_LEFT] != 0)  /*左限位*/
    {
        if(g_vfd_ctrl.end != 0)
        {
            motor_stop_ctl(CODE_END);
            pump_ctl_set_value(0 , 500);
            g_vfd_ctrl.end = 0;
        }
        else 
        {
            if(motor_target_current_dir() == 0){/*正在向左运动*/
                ext_motor_reverse();
                ext_send_report_to_data(0,0xFF,0xFF,0xFF,0xFF,1,motor_mode_get());
            } 
                
        }
    }

    if(g_vfd_ctrl.flag[IO_ID_LIMIT_RIGHT] != 0) /*右限位*/
    {
        if(g_vfd_ctrl.end != 0)
        {
            motor_stop_ctl(CODE_END);
            pump_ctl_set_value(0 , 500);
            g_vfd_ctrl.end = 0;
        }
        else
        {
            if(motor_target_current_dir() == 1){/*正在向右运动*/
                ext_motor_reverse();
                ext_send_report_to_data(0,0xFF,0xFF,0xFF,0xFF,0,motor_mode_get());
            } 
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
                g_vfd_io_tab[i].now_ticks += MAIN_CTL_PERIOD;
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
            pump_ctl_set_value(0 , 0);
        }
    }
        
        
    //右限位长时间触发，异常
    if(g_vfd_io_tab[IO_ID_LIMIT_RIGHT].now_ticks >= g_vfd_io_tab[IO_ID_LIMIT_RIGHT].exceed_ticks)
    {
        g_vfd_ctrl.err |= ERROR_RIGHT_KEY;
        if(motor_is_working())
        {
            motor_stop_ctl(CODE_LIMIT_TIMEOUT);
            pump_ctl_set_value(0 , 0);
        }
    } 
        

    /*左右限位同时触发*/
    if((g_vfd_ctrl.flag[IO_ID_LIMIT_LEFT] == 1) && (g_vfd_ctrl.flag[IO_ID_LIMIT_RIGHT] == 1))
    {
        g_vfd_ctrl.err |= ERROR_DOUBLE_KEY;
        if(motor_is_working())
        {
            motor_stop_ctl(CODE_LIMIT_DOUBLE);
            pump_ctl_set_value(0 , 0);
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
                int value = pump_ctl_toggle_value();
                ext_send_report_to_data(0,0,0xFF,value,0xFF,0xFF,motor_mode_get());
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
                pump_ctl_set_value(1 , 0);
            }  
            if(g_vfd_ctrl.flag[IO_ID_PUMP_STOP] != 0) /*关水*/
            {
                pump_ctl_set_value(0 , 0);
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
                g_vfd_io_tab[i].now_ticks += MAIN_CTL_PERIOD;
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

static uint8_t is_power_off(int voltage , uint32_t timeout_time)
{
    static uint32_t power_off_start_time = 0;
    if(voltage < 150)  /*电压低于150V，则认为掉电*/
    {
        if(power_off_start_time == 0){
            power_off_start_time = HAL_GetTick(); /*记录保护时间*/
        }
        else{
            uint32_t elapsed_time = HAL_GetTick() - power_off_start_time;
            if(elapsed_time >= timeout_time)
                return 1;
        }
    }
    else
    {
        power_off_start_time = 0;
    }
    return 0;
}

static uint8_t check_voltage_status(int voltage, int low_threshold, int high_threshold, uint32_t timeout_time)
{
    static uint32_t voltage_abnormal_start_time = 0; // 记录异常开始时间
    if (voltage < low_threshold) {
        if (voltage_abnormal_start_time == 0) {
            voltage_abnormal_start_time = HAL_GetTick(); // 第一次低于下限，记录时间
        } else {
            uint32_t elapsed_time = HAL_GetTick() - voltage_abnormal_start_time;
            if (elapsed_time >= timeout_time) {
                return 1; // 持续低于下限
            }
        }
    } else if (voltage > high_threshold) {
        if (voltage_abnormal_start_time == 0) {
            voltage_abnormal_start_time = HAL_GetTick(); // 第一次高于上限，记录时间
        } else {
            uint32_t elapsed_time = HAL_GetTick() - voltage_abnormal_start_time;
            if (elapsed_time >= timeout_time) {
                return 2; // 持续高于上限
            }
        }
    } else {
        // 电压恢复正常区间，重置计时器
        voltage_abnormal_start_time = 0;
    }

    return 0; // 电压在正常范围内或未超时
}


void scan_voltage(void)
{
    //根据GB/T 12325-2008，220V单相的允许偏差为标称电压的+7%（上限）和-10%（下限），即198V至235.4V‌
    //实际使用为220V±20% = 176V至264V
    //电压小于150V认为掉电
    static uint32_t last_check_time = 0;
    uint32_t now = HAL_GetTick();
    if (now - last_check_time < 200) {
        return ; 
    }

    if(motor_speed_is_const() == 0) //变速时，由于母线电压不稳定，此时不再检测电压
        return ;

    last_check_time = now;
    /*获取电压保护参数和掉电异常参数*/
    //uint8_t voltage_protect = 0;
    //param_get(PARAM0X02, PARAM_VOLTAGE_ADJUST, &voltage_protect); /*更新电压调节参数*/
    uint8_t power_off_time = 0;
    param_get(PARAM0X03, PARAM_POWER_OFF_TIME, &power_off_time); /*允许掉电的最长时间，单位0.1秒，最大50*/
    
    uint16_t timeout = power_off_time * 100;  /*转换成毫秒*/

    int voltage = bsp_get_voltage();
 
    if(is_power_off(voltage,timeout) == 1){
        g_vfd_voltage_flag = 3;
        if(motor_is_running()){
            pump_ctl_set_value(0 , 500);
            motor_stop_ctl(CODE_POWER_OFF);
        }
    }else{
        uint8_t voltage_status = check_voltage_status(voltage,176 , 264,timeout);
        if(voltage_status != 0){
            if(motor_is_running()){
                pump_ctl_set_value(0 , 500);
                motor_stop_ctl((stopcode_t)(voltage_status + 4));
            }
        }
        g_vfd_voltage_flag = voltage_status;
    }
}

void inout_init(void) 
{
    /*17个输入信号线*/
    bsp_io_init_input();
    /*3个输出信号线*/
    bsp_io_init_output();
    /*IPM故障，中断检测*/
    bsp_ipm_vfo_init();
    
    return ;
}


void inout_scan(void)
{
    /*step 1 . 极性*/
    io_scan_active_polarity();
    /*step 2 . 调试模式*/
    io_scan_debug();
    /*step 3 . 速度*/
    io_scan_speed();
    /*step 4 . 断丝*/
    io_scan_wire();
    /*step 5 . 左右限位,超程和加工结束*/
    io_scan_direction();
    /*step 6 . 开关运丝和水泵*/
    io_scan_onoff();
    /*step 7 . 电压检测*/
    scan_voltage();
    /*step 8 . 错误处理*/
    update_err();

}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == GPIO_PIN_7)
    {
        if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) == GPIO_PIN_RESET)
        {
            g_ipm_vfo_flag = 1;
            //motor_stop_ctl(CODE_IPM_VFO);
        }
        else
            g_ipm_vfo_flag = 0;
    }
}

