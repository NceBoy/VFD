#include "main.h"
#include "bsp_tmr.h"
#include "motor.h"
#include "log.h"
#include "cordic.h"
#include "motor.h"
#include "vfd_param.h"


extern TIM_HandleTypeDef htim8;

#define ROUND_TO_UINT(x)        ((unsigned int)(x + 0.5))

typedef enum
{
    motor_in_idle,
    motor_in_run,
    motor_in_reverse,
    motor_in_break,
}motor_status_enum;
/*为了计算精度，统一为float*/
typedef struct
{
    motor_status_enum   motor_status;
    unsigned int        motor_dir;
    float               ratio;
    float               angle;        /*当前的角度，一直累加的值，用于计算sin值*/
    float               target_should_be;
    float               target_freq;
    float               current_freq;
    float               next_step_freq;
}motor_ctl_t;

typedef struct 
{
    float start_freg;
    unsigned int acceleration_time_us;
    unsigned int deceleration_time_us;
}motor_para_t;


static motor_para_t g_motor_param;
static motor_ctl_t g_motor_real;

static void motor_param_get(void)
{
    uint8_t value = 0;
    pullOneItem(PARAM0X02, PARAM_ACCELERATION_TIME, &value);
    g_motor_param.acceleration_time_us = value * 100 * 1000;
    pullOneItem(PARAM0X02, PARAM_DECELERATION_TIME, &value);
    g_motor_param.deceleration_time_us = value * 100 * 1000;
    pullOneItem(PARAM0X03, PARAM_START_FREQ, &value);
    g_motor_param.start_freg = (float)value;
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
    g_motor_real.target_freq = g_motor_param.start_freg;
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
    g_motor_real.target_freq = g_motor_param.start_freg;
    g_motor_real.motor_status = motor_in_break;
}

void motor_break(void)
{
    //bsp_tmr_stop();
    bsp_tmr_update_compare(PWM_RESOLUTION / 2 , 0 , 0);
    g_motor_real.motor_status = motor_in_idle;
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
    g_motor_real.ratio = 0.9;
    g_motor_real.motor_dir = dir;
    g_motor_real.angle = 0.0f;
    g_motor_real.current_freq = 0.0f;
    g_motor_real.next_step_freq = g_motor_param.start_freg;
    g_motor_real.target_should_be = target_freq;
    g_motor_real.target_freq = target_freq;
    /*step 3 . start timer*/
    bsp_tmr_start();
}

static void motor_update_spwm(void)
{   
    // 计算三相占空比
    unsigned short phaseA = 0;
    unsigned short phaseB = 0;
    unsigned short phaseC = 0;
    phaseA = (unsigned short)(g_motor_real.ratio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle)));
    if(g_motor_real.motor_dir == 0)
    {
        phaseC = (unsigned short)(g_motor_real.ratio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle + PHASE_SHIFT_120)));
        phaseB = (unsigned short)(g_motor_real.ratio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle - PHASE_SHIFT_120)));        
    }
    else
    {
        phaseC = (unsigned short)(g_motor_real.ratio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle - PHASE_SHIFT_120)));
        phaseB = (unsigned short)(g_motor_real.ratio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle + PHASE_SHIFT_120)));         
    }


    bsp_tmr_update_compare(phaseA , phaseB , phaseC);

    g_motor_real.current_freq = g_motor_real.next_step_freq;

    /*计算下一步的角度*/
    float delta = PI_2 * PWM_CRCLE * g_motor_real.current_freq;
    g_motor_real.angle += delta;
    if (g_motor_real.angle >= PI_2) g_motor_real.angle -= PI_2;

    /*计算下一步的频率*/
    if(motor_arrive_freq(g_motor_real.target_freq))
        g_motor_real.next_step_freq = g_motor_real.target_freq;
    else
        g_motor_real.next_step_freq = motor_calcu_next_step_freq(   g_motor_param.start_freg , 
                                                                    g_motor_param.acceleration_time_us ,
                                                                    g_motor_param.deceleration_time_us ,
                                                                    (unsigned int)(PWM_CRCLE * 1000000 ),
                                                                    g_motor_real.current_freq,
                                                                    g_motor_real.target_freq);

}



unsigned int interrupt_times = 0;
 void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
 {
    if(htim->Instance == TIM1)
    {
        interrupt_times++;
        if(g_motor_real.motor_status == motor_in_idle)
            return;
        if((g_motor_real.motor_status == motor_in_reverse) &&
            (motor_arrive_freq(g_motor_param.start_freg) == 1))
        {
            motor_reverse_recovery();
        }
        else if((g_motor_real.motor_status == motor_in_break) &&
                (motor_arrive_freq(g_motor_param.start_freg) == 1))
        {
            motor_break();
        }
        motor_update_spwm();
        
    }
 }