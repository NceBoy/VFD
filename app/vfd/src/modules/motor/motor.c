#include "main.h"
#include "bsp_tmr.h"
#include "motor.h"
#include "log.h"
#include "cordic.h"
#include "motor.h"


extern TIM_HandleTypeDef htim8;
/*为了计算精度，统一为float*/
typedef struct
{
    motor_status_enum   motor_status;
    unsigned int        motor_dir;
    float               ratio;
    float               angle;        /*当前的角度，一直累加的值，用于计算sin值*/
    float               target_freq;
    float               current_freq;
    float               next_step_freq;
}motor_ctl_t;

#define ROUND_TO_UINT(x)        ((unsigned int)(x + 0.5))

static motor_ctl_t g_motor_real;

static void motor_current_freq_set(float freq)
{
    g_motor_real.current_freq = freq;
}

void motor_target_info_update(float target_freq)
{
    g_motor_real.target_freq = target_freq;
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
                                        unsigned int one_step_time_us,
                                        float current_freq,
                                        float target_freq)
{
    float next_step_freq = 0.0f;
    float acceleration_hz_1us = (50.0f - startup_freq ) / (float)acceleration_time_us;
    float one_step_hz = acceleration_hz_1us * one_step_time_us;
    if(float_equal_in_step(current_freq , target_freq, one_step_hz))
        return target_freq;
    if(target_freq > current_freq)
        next_step_freq = current_freq + one_step_hz ;
    else
        next_step_freq = current_freq - one_step_hz;
    return next_step_freq;
}

void motor_status_set(motor_status_enum status)
{
    g_motor_real.motor_status = status;
}

motor_status_enum motor_status_get(void)
{
    return g_motor_real.motor_status;
}

float motor_current_freq_get(void)
{
    return g_motor_real.current_freq;
}

void motor_target_freq_update(float target_freq)
{
    g_motor_real.target_freq = target_freq;
}

void motor_reverse(void)
{
    if(g_motor_real.motor_dir > 0)
        g_motor_real.motor_dir = 0;
    else g_motor_real.motor_dir++;
}

unsigned int motor_arrive_freq(float freq)
{
    return float_equal_in_step(g_motor_real.current_freq , freq, 0.01);
}

void motor_init(void)
{
    bsp_tmr_init();
}


void motor_start(unsigned int dir , float target_freq)
{
    /*step 1 . init tmr*/
   
    /*step 2 . init motor value*/
    g_motor_real.motor_status = motor_in_run;
    g_motor_real.ratio = 0.9;
    g_motor_real.motor_dir = dir;
    g_motor_real.angle = 0.0f;
    g_motor_real.current_freq = 0.0f;
    g_motor_real.next_step_freq = 5.0f;
    motor_target_freq_update(target_freq);
    /*step 3 . start timer*/
    bsp_tmr_start();
}

void motor_update_spwm(void)
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

    motor_current_freq_set(g_motor_real.next_step_freq);


    /*计算下一步的角度*/
    float delta = PI_2 * PWM_CRCLE * g_motor_real.current_freq;
    g_motor_real.angle += delta;
    if (g_motor_real.angle >= PI_2) g_motor_real.angle -= PI_2;

    /*计算下一步的频率*/
    g_motor_real.next_step_freq = motor_calcu_next_step_freq(   5 , 
                                                                800 * 1000 ,
                                                                (unsigned int)(PWM_CRCLE * 1000000 ),
                                                                g_motor_real.current_freq,
                                                                g_motor_real.target_freq);

}

void motor_break(void)
{
    bsp_tmr_stop();
    bsp_tmr_update_compare(PWM_RESOLUTION / 2 , 0 , 0);
}

unsigned int interrupt_times = 0;
 void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
 {
    if(htim->Instance == TIM8)
    {
        motor_update_spwm();
        interrupt_times++;
    }
 }