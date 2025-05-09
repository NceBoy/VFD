#include "main.h"
#include "bsp_tmr.h"
#include "motor.h"
#include "log.h"
#include "cordic.h"
#include "motor.h"
#include "vfd_param.h"

static TIM_HandleTypeDef htim7; /*开高频延时*/

#define ROUND_TO_UINT(x)        ((unsigned int)(x + 0.5))
#define RADIO_MAX               (0.9f)

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
    float start_freg;           /*启动频率*/
    float open_freq;            /*开高频最低频率*/
    unsigned int radio;                /*转矩提升*/
    unsigned int delay;                /*高频延时*/
    unsigned int economy;                /*自动省电*/

    unsigned int vari_freq;                 /*变频关高频标志位*/
    unsigned int acceleration_time_us;      /*加速时间*/
    unsigned int deceleration_time_us;      /*减速时间*/
}motor_para_t;


static motor_para_t g_motor_param;
static motor_ctl_t g_motor_real;        /*开高频标志位*/
static float g_radio_rate[7] = {0.0f,0.1f,0.2f,0.25f,0.3f,0.35f,0.4f}; 

static void motor_param_get(void)
{
    uint8_t value = 0;
    pullOneItem(PARAM0X03, PARAM_START_FREQ, &value);  /*启动频率*/
    g_motor_param.start_freg = (float)value;

    pullOneItem(PARAM0X02, PARAM_MIN_OPEN_FREQ, &value);    /*开高频频率*/
    g_motor_param.open_freq = (float)value;

    pullOneItem(PARAM0X02, PARAM_LOW_FREQ_TORQUE_BOOST, &value);    /*低频转矩提升*/
    g_motor_param.radio = value;

    pullOneItem(PARAM0X02, PARAM_HIGH_FREQ_DELAY, &value);    /*开高频延时*/
    g_motor_param.delay = value;

    pullOneItem(PARAM0X02, PARAM_AUTO_ECONOMY_PERCENT, &value);    /*自动省电*/
    g_motor_param.economy = value;

    pullOneItem(PARAM0X02, PARAM_VARI_FREQ_CLOSE, &value);    /*变频关高频*/
    g_motor_param.vari_freq = value;

    pullOneItem(PARAM0X02, PARAM_ACCELERATION_TIME, &value); /*加速时间*/
    g_motor_param.acceleration_time_us = value * 100 * 1000;

    pullOneItem(PARAM0X02, PARAM_DECELERATION_TIME, &value); /*减速时间*/
    g_motor_param.deceleration_time_us = value * 100 * 1000;


}

static float radio_from_freq(float freq)
{
    float radio = 0.0f;
    if(freq > 50.0f)
        return RADIO_MAX;
    if(g_motor_param.radio > 6)
        g_motor_param.radio = 6;
    radio = g_radio_rate[g_motor_param.radio];
    return ((1 - radio) / 50.0f * freq + radio) * RADIO_MAX;
}

static int open_high_frequery_init(void)
{
    /* Peripheral clock enable */
    __HAL_RCC_TIM7_CLK_ENABLE();

    TIM_MasterConfigTypeDef sMasterConfig = {0};
    htim7.Instance = TIM7;  /*160MHz  32000  5000Hz*/
    htim7.Init.Prescaler = 32000 - 1;
    htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim7.Init.Period = (g_motor_param.delay * 100 * 5) - 1 ;
    htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim7);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig);

    /* TIM7 interrupt Init */
    HAL_NVIC_SetPriority(TIM7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);
    return 0;
}

static void high_frequery_open(void)
{
    if(g_motor_real.high_status == 0)
    {
        htim7.Instance->CNT = 0;
        __HAL_TIM_ENABLE_IT(&htim7, TIM_IT_UPDATE);
        __HAL_TIM_ENABLE(&htim7);   
        g_motor_real.high_status = 1; 
    }
}

static void high_frequery_close(void)
{
    if(g_motor_real.high_status == 1)
    {
        htim7.Instance->CNT = 0;
        __HAL_TIM_DISABLE_IT(&htim7, TIM_IT_UPDATE);
        __HAL_TIM_DISABLE(&htim7);  
        g_motor_real.high_status = 0;
    }
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
    bsp_tmr_update_compare(PWM_RESOLUTION / 2 , 0 , 0);
    g_motor_real.motor_status = motor_in_idle;
    g_motor_real.break_release = 10 * 1000;
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
    open_high_frequery_init();
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
    g_motor_real.next_step_freq = g_motor_param.start_freg;
    g_motor_real.target_should_be = target_freq;
    g_motor_real.target_freq = target_freq;
    /*step 3 . start timer*/
    bsp_tmr_start();
}

/*每次中断调用一次*/
static void motor_update_spwm(void)
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
        g_motor_real.next_step_freq = motor_calcu_next_step_freq(   g_motor_param.start_freg , 
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
        if((g_motor_param.vari_freq) &&(g_motor_real.freq_arrive == 0)) /*变频关高频*/
        {
            /*关*/
            high_frequery_close();
            /*IO操作*/
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
        /*IO操作*/

    }

}

unsigned int interrupt_times = 0;
 void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
 {
    if(htim->Instance == TIM1)
    {
        interrupt_times++;
        if(g_motor_real.break_release != 0)
            g_motor_real.break_release --;
        else
            bsp_tmr_stop();
        
        
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
        high_freq_control();     
    }
    else if(htim->Instance == TIM7)
    {
        /*开高频IO操作*/
    }
 }