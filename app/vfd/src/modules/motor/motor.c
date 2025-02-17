#include "main.h"
#include "motor.h"
#include "log.h"


/*为了计算精度，统一为float*/
typedef struct
{
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

void motor_start(void)
{
    g_motor_real.current_freq = 15.0f;
    g_motor_real.next_step_freq = 15.0f;
    g_motor_real.target_freq = 15.0f;
    motor_target_info_update(25);
}

void motor_test(void)
{
    /*根据上一步计算的next_setp_freq更新sin的值*/
    //motor_current_freq_set(g_motor_real.next_step_freq);
    //g_motor_real.next_step_freq = motor_calcu_next_step_freq( 5 , 800 * 1000 ,100,
    //                            g_motor_real.current_freq,g_motor_real.target_freq);
    g_motor_real.next_step_freq = 15.9958f;
    g_motor_real.target_freq = 16.0001209f;
    logdbg("next freq = %d \n",float_equal(g_motor_real.next_step_freq, g_motor_real.target_freq));
}