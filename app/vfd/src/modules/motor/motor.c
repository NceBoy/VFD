#include "main.h"
#include "bsp_tmr.h"
#include "motor.h"
#include "log.h"
#include "cordic.h"


extern TIM_HandleTypeDef htim8;
/*为了计算精度，统一为float*/
typedef struct
{
    float angle;        /*当前的角度，一直累加的值，用于计算sin值*/
    float target_freq;
    float current_freq;
    float next_step_freq;
}motor_ctl_t;

#define ROUND_TO_UINT(x)        ((unsigned int)(x + 0.5))

static motor_ctl_t g_motor_real = {0};

void motor_current_freq_set(float freq)
{
    g_motor_real.current_freq = freq;
}

void motor_target_info_update(float target)
{
    g_motor_real.target_freq = target;
}

static unsigned int float_equal(float a , float b)
{
    if(a > b)
    {
        if((a-b) < 0.01)
            return 1;
    }
    else
    {
        if((b-a) < 0.01)
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
    float acceleration_hz_1us = (50.0f - (float)startup_freq ) / (float)acceleration_time_us;
    if(target_freq > current_freq)
        next_step_freq = current_freq + acceleration_hz_1us * one_step_time_us ;
    else
        next_step_freq = current_freq - acceleration_hz_1us * one_step_time_us;
    return next_step_freq;
}

void motor_start(float target_freq)
{
    /*step 1 . init motor value*/
    g_motor_real.angle = 0.0f;
    g_motor_real.current_freq = 0.0f;
    g_motor_real.next_step_freq = 5.0f;
    motor_target_info_update(target_freq);
    /*step 2 . start timer*/
}

void motor_update_spwm(unsigned int dir , float modulation_ratio)
{   
    // 计算三相占空比
    unsigned short phaseA = 0;
    unsigned short phaseB = 0;
    unsigned short phaseC = 0;
    phaseA = (unsigned short)(modulation_ratio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle)));
    if(dir == 0)
    {
        phaseB = (unsigned short)(modulation_ratio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle - PHASE_SHIFT_120)));
        phaseC = (unsigned short)(modulation_ratio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle + PHASE_SHIFT_120)));        
    }
    else
    {
        phaseC = (unsigned short)(modulation_ratio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle - PHASE_SHIFT_120)));
        phaseB = (unsigned short)(modulation_ratio * (PWM_RESOLUTION / 2) * (1 + cordic_sin(g_motor_real.angle + PHASE_SHIFT_120)));         
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

    logdbg("%d:%d:%d\n",phaseA , phaseB , phaseC);
}








 void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
 {
    if(htim->Instance == TIM8)
    {

    }
 }