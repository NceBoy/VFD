#include "main.h"
#include "bsp_tmr.h"
#include "bsp_io.h"
#include "motor.h"
#include "log.h"
#include "cordic.h"
#include "motor.h"
#include "param.h"
#include "hmi.h"

static TIM_HandleTypeDef htim7; /*开高频延时*/

#define ROUND_TO_UINT(x)        ((unsigned int)(x + 0.5))
#define RADIO_MAX               (0.95f)

typedef enum
{
    motor_in_idle,          /*空闲*/
    motor_in_release,       /*刹车释放*/
    motor_in_run,           /*正常运行*/
    motor_in_reverse,       /*反向*/
    motor_in_break,         /*刹车*/
}motor_status_enum;
/*为了计算精度，统一为float*/
typedef struct
{
    motor_status_enum   motor_status;
    unsigned int        motor_dir;
    unsigned int        freq_arrive;
    unsigned int        break_release;
    unsigned int        high_status;  /*高频状态*/
    float               angle;        /*当前的角度，一直累加的值，用于计算sin值*/
    float               target_should_be;
    float               target_freq;
    float               current_freq;
    float               next_step_freq;
}motor_ctl_t;

typedef struct 
{
    float start_freq;           /*启动频率*/
    float open_freq;            /*开高频最低频率*/
    unsigned int open_freq_delay;    /*开高频延时*/
    unsigned int vari_freq_close;   /*变频关高频标志位*/
    
    unsigned int radio;                /*转矩提升*/
    unsigned int auto_economy;          /*自动省电*/
    unsigned int start_dir;             /*启动方向*/
    
    unsigned int acceleration_time_us;      /*加速时间*/
    unsigned int deceleration_time_us;      /*减速时间*/
}motor_para_t;

static int g_highfreq_delay = 0;
static motor_para_t g_motor_param;
static motor_ctl_t g_motor_real;        /*开高频标志位*/
static float g_radio_rate[7] = {0.0f,0.1f,0.2f,0.25f,0.3f,0.35f,0.4f}; 

static void motor_update_compare(void);

static void motor_param_get(void)
{
    uint8_t value = 0;
    /*启动频率*/
    param_get(PARAM0X03, PARAM_START_FREQ, &value);
    g_motor_param.start_freq = (float)value;
    /*开高频频率*/
    param_get(PARAM0X02, PARAM_HIGH_FREQ, &value);
    g_motor_param.open_freq = (float)value;
    /*开高频延时，单位：0.1秒*/
    param_get(PARAM0X02, PARAM_HIGH_FREQ_DELAY, &value);
    g_motor_param.open_freq_delay = value;
    /*变频关高频*/
    param_get(PARAM0X02, PARAM_VARI_FREQ_CLOSE, &value);
    g_motor_param.vari_freq_close = value;
    /*转矩提升*/
    param_get(PARAM0X02, PARAM_ORQUE_BOOST, &value);
    g_motor_param.radio = value;
    /*自动省电*/
    param_get(PARAM0X02, PARAM_AUTO_ECONOMY, &value);
    g_motor_param.auto_economy = value;
    /*启动方向:0:之前的方向，1:向左 ， 2:向右 */
    param_get(PARAM0X03, PARAM_START_DIRECTION, &value);
    g_motor_param.start_dir = value;
    /*加速时间，单位0.1秒*/
    param_get(PARAM0X02, PARAM_ACCE_TIME, &value);
    g_motor_param.acceleration_time_us = value* 100 * 1000;
    /*减速时间，单位0.1秒*/
    param_get(PARAM0X02, PARAM_DECE_TIME, &value);
    g_motor_param.deceleration_time_us = value* 100 * 1000;

}

static float radio_from_freq(float freq)
{
    if(g_motor_real.motor_status == motor_in_break)
        return RADIO_MAX;
    if(freq > 50.0f)
        return RADIO_MAX;
    float radio = 0.0f;

    if(g_motor_param.radio > 6)
        g_motor_param.radio = 6;
    radio = g_radio_rate[g_motor_param.radio];
    return ((1 - radio) / 50.0f * freq + radio) * RADIO_MAX;
}


static void high_frequery_open(void)
{
    if(g_motor_real.high_status == 0)
    {
        g_motor_real.high_status = 1; 
        g_highfreq_delay = g_motor_param.open_freq_delay * 1000; //单位:100us
    }
}

static void high_frequery_close(void)
{
    if(g_motor_real.high_status == 1)
    {
        g_motor_real.high_status = 0;
        HIGH_FREQ_DISABLE;
    }
}

unsigned char motor_is_open_freq(void)
{
    return g_motor_real.high_status;
}

void motor_target_info_update(float target_freq)
{
    switch(g_motor_real.motor_status)
    {
        case motor_in_reverse:{
            g_motor_real.target_should_be = target_freq;
        }break;
        case motor_in_run:{
            g_motor_real.target_should_be = target_freq;
            g_motor_real.target_freq = target_freq;
        }break;
        default:break;
    }
}

int motor_is_working(void)
{
    return (g_motor_real.motor_status == motor_in_idle) ?  0 : 1;
}

int motor_target_current_get(void)
{
    return (int)g_motor_real.target_should_be;
}

int motor_target_current_dir(void)
{
    return (int)g_motor_real.motor_dir;
}

void motor_reverse_start(void)
{
    g_motor_real.target_freq = g_motor_param.start_freq;
    g_motor_real.motor_status = motor_in_reverse;
}

static void motor_reverse_recovery(void)
{
    if(g_motor_real.motor_dir > 0)  /*内置定义0:向左，1:向右，如果不对，交换电机的任意2相电源线*/
        g_motor_real.motor_dir = 0;
    else g_motor_real.motor_dir++;

    g_motor_real.target_freq = g_motor_real.target_should_be;

    g_motor_real.motor_status = motor_in_run;
}

void motor_break_start(void)
{
    if((g_motor_real.motor_status == motor_in_break) ||
       (g_motor_real.motor_status == motor_in_release))
        return ;
    g_motor_real.target_freq = g_motor_param.start_freq;
    g_motor_real.motor_status = motor_in_break;

    high_frequery_close();
    EXT_PUMP_DISABLE;
}

void motor_break(void)
{
    //bsp_tmr_update_compare(PWM_RESOLUTION / 2 , PWM_RESOLUTION / 2 , PWM_RESOLUTION / 2);
    //bsp_tmr_update_compare(PWM_RESOLUTION / 2 , 0 , 0);
    //motor_update_compare();
    bsp_tmr_break();
    g_motor_real.motor_status = motor_in_release;
    g_motor_real.break_release = 10 * 1000;   //单位:100us，实际时间1秒钟
}


static unsigned int float_equal_in_step(float a , float b, float step)
{
    if(a > b)
    {
        if((a-b) < step)
            return 1;
    }
    else
    {
        if((b-a) < step)
            return 1;        
    }
    return 0;
}

/*(T型加减速,下一步频率计算)*/
static float motor_calcu_next_step_freq(float startup_freq , 
                                        unsigned int acceleration_time_us ,
                                        unsigned int deceleration_time_us ,
                                        unsigned int one_step_time_us,
                                        float current_freq,
                                        float target_freq)
{
    float next_step_freq = 0.0f;
    float step_hz_1us = 0.0f;

    if(target_freq > current_freq) /*加速*/
        step_hz_1us = (50.0f - startup_freq ) / (float)acceleration_time_us;
    else /*减速*/
        step_hz_1us = (50.0f - startup_freq ) / (float)deceleration_time_us;

    float one_step_hz = step_hz_1us * one_step_time_us;
    if(float_equal_in_step(current_freq , target_freq, one_step_hz))
        return target_freq;
    if(target_freq > current_freq)
        next_step_freq = current_freq + one_step_hz ;
    else
        next_step_freq = current_freq - one_step_hz;
    return next_step_freq;
}



static unsigned int motor_arrive_freq(float freq)
{
    return float_equal_in_step(g_motor_real.current_freq , freq, 0.01);
}

void motor_init(void)
{
    bsp_tmr_init();
    if(g_motor_real.motor_status != motor_in_idle)
        bsp_tmr_start();
}


void motor_start(unsigned int dir , float target_freq)
{
    /*step 1 . init tmr*/
    
    motor_param_get();
    /*step 2 . init motor value*/
    g_motor_real.motor_status = motor_in_run;
    g_motor_real.motor_dir = dir;
    g_motor_real.angle = 0.0f;
    g_motor_real.current_freq = 0.0f;
    g_motor_real.next_step_freq = g_motor_param.start_freq;
    g_motor_real.target_should_be = target_freq;
    g_motor_real.target_freq = target_freq;
    /*step 3 . start timer*/
    bsp_tmr_start();
    hmi_clear_menu();
}

static void motor_update_compare(void)
{
    // 计算三相占空比
    unsigned short phaseA = 0;
    unsigned short phaseB = 0;
    unsigned short phaseC = 0;

    float radio = radio_from_freq(g_motor_real.current_freq);
    if(radio > RADIO_MAX) /*保护IPM模块*/
        radio = RADIO_MAX;

    phaseA = (unsigned short)(radio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle)));
    if(g_motor_real.motor_dir == 0)
    {
        phaseC = (unsigned short)(radio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle + PHASE_SHIFT_120)));
        phaseB = (unsigned short)(radio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle - PHASE_SHIFT_120)));        
    }
    else
    {
        phaseC = (unsigned short)(radio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle - PHASE_SHIFT_120)));
        phaseB = (unsigned short)(radio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle + PHASE_SHIFT_120)));         
    }
    bsp_tmr_update_compare(phaseA , phaseB , phaseC); 
}
/*每次中断调用一次*/
static void motor_update_spwm(void)
{   
    motor_update_compare();

    g_motor_real.current_freq = g_motor_real.next_step_freq;

    /*计算下一步的角度*/
    float delta = PI_2 * TMR_INT_PERIOD_US * g_motor_real.current_freq / 1000000;
    g_motor_real.angle += delta;
    if (g_motor_real.angle >= PI_2) g_motor_real.angle -= PI_2;

    /*计算下一步的频率*/
    if(motor_arrive_freq(g_motor_real.target_freq)){
        g_motor_real.next_step_freq = g_motor_real.target_freq;
        g_motor_real.freq_arrive = 1;
    }
    else{
        g_motor_real.freq_arrive = 0;
        g_motor_real.next_step_freq = motor_calcu_next_step_freq(g_motor_param.start_freq ,
            g_motor_param.acceleration_time_us ,
            g_motor_param.deceleration_time_us ,
            TMR_INT_PERIOD_US,
            g_motor_real.current_freq,
            g_motor_real.target_freq);
    }
}

static void high_freq_control(void)
{
    if(g_motor_real.current_freq > g_motor_param.open_freq)
    {
        if((g_motor_param.vari_freq_close) &&(g_motor_real.freq_arrive == 0)) /*变频关高频*/
        {
            /*关*/
            high_frequery_close();
        }
        else
        {
            /*开*/
            high_frequery_open();
        }
    }
    else
    {   /*关*/
        high_frequery_close();
    }

}

unsigned int interrupt_times = 0;
 void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
 {
    if(htim->Instance == TIM1)
    {
        interrupt_times++;
        if(g_motor_real.high_status) /*开高频延时*/
        {
            if(g_highfreq_delay > 0)
            {
                g_highfreq_delay--;
                if(g_highfreq_delay == 0)
                    HIGH_FREQ_ENABLE;
            }
        }

        if(g_motor_real.motor_status == motor_in_idle)
            return;

        if(g_motor_real.motor_status == motor_in_release)
        {
            if(g_motor_real.break_release != 0)
                g_motor_real.break_release --;
            else
            {
                bsp_tmr_stop();
                g_motor_real.motor_status = motor_in_idle;
                high_frequery_close();
                EXT_PUMP_DISABLE;
            }
                
            return ;            
        }

        if((g_motor_real.motor_status == motor_in_reverse) &&
            (motor_arrive_freq(g_motor_param.start_freq) == 1))
        {
            motor_reverse_recovery();
        }
        else if((g_motor_real.motor_status == motor_in_break) &&
                (motor_arrive_freq(g_motor_param.start_freq) == 1))
        {
            motor_break();
        }
        motor_update_spwm(); 
        high_freq_control();
    }
 }