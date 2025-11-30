#include "main.h"
#include "service_data.h"
#include "bsp_tmr.h"
#include "bsp_io.h"
#include "motor.h"
#include "log.h"
#include "cordic.h"
#include "param.h"
#include "inout.h"
#include "hmi.h"

#define  HIGH_OPEN_DELAY_MIN      3       /*开高频最小延时，单位0.1秒*/
#define  HIGH_OPEN_DELAY_MAX      30       /*开高频最小延时，单位0.1秒*/

#define ROUND_TO_UINT(x)        ((unsigned int)(x + 0.5))
#define RADIO_MAX               (0.95f)

typedef enum
{
    motor_in_idle,          /*空闲*/
    motor_in_dc_brake,      /*直流刹车*/
    motor_in_run,           /*正常运行*/
    motor_in_reverse,       /*反向减速阶段*/
    motor_in_brake,         /*刹车减速阶段*/
}motor_status_enum;
/*为了计算精度，统一为float*/
typedef struct
{
    motor_status_enum   motor_status;
    unsigned int        motor_dir;
    unsigned int        freq_arrive;
    unsigned int        dc_brake_time;
    float               angle;        /*当前的角度，一直累加的值，用于计算sin值*/
    float               target_should_be;
    float               target_freq;
    float               current_freq;
    float               next_step_freq;
}motor_ctl_t;

typedef struct 
{
    unsigned char        real_status;  /*高频状态*/
    unsigned char        ctrl_value;
    unsigned int         ctrl_delay;
}motor_high_freq_t;

typedef struct 
{
    unsigned char need_ctl;
    unsigned int  ctl_delay;
}eeprom_ctl_t;

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

static motor_high_freq_t g_high_freq;

static motor_para_t g_motor_param;
static motor_ctl_t g_motor_real;   
static eeprom_ctl_t g_eeprom_ctl;

static float g_radio_rate[7] = {0.0f,0.1f,0.2f,0.25f,0.3f,0.35f,0.4f}; 

static void motor_update_compare(void);

static void motor_eeprom_ctl(void)
{
    g_eeprom_ctl.need_ctl = 1;
    g_eeprom_ctl.ctl_delay = 200; /*200ms后操作*/
}

void motor_save_check(int period)
{
    if(g_eeprom_ctl.need_ctl == 0)
        return ;
    if(g_eeprom_ctl.ctl_delay > period)
        g_eeprom_ctl.ctl_delay -= period;
    else
    {
        g_eeprom_ctl.need_ctl = 0;
        g_eeprom_ctl.ctl_delay = 0;
        param_dir_save(g_motor_real.motor_dir);    /*保存当前运行的方向*/
    }
}

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
    if(value < 4)
        value = 4;
    g_motor_param.acceleration_time_us = value* 100 * 1000;
    
    /*减速时间，单位0.1秒，经实际测试，50HZ换向时，减速时间最小0.4秒，80Hz换向时，减速时间最小0.7秒*/
    param_get(PARAM0X02, PARAM_DECE_TIME, &value);
    if(value < 4)
        value = 4;    
    g_motor_param.deceleration_time_us = value* 100 * 1000;
}

static float radio_from_freq(float freq)
{
    //if(freq > 50.0f)
    //    return RADIO_MAX;
    float radio = 0.0f;

    if(g_motor_param.radio > 6)
        g_motor_param.radio = 6;
    radio = g_radio_rate[g_motor_param.radio];
    return ((1 - radio) / 50.0f * freq + radio) * RADIO_MAX;
}

/*高频控制:1 开  2关*/
static void high_frequery_ctl(int value)
{
    if((g_high_freq.real_status == value) || (g_high_freq.ctrl_value == value))
        return ;

    g_high_freq.ctrl_value = value;
    if(value == 1) /*延时开*/
    {
        unsigned int real_delay = 0;
        if(g_motor_param.open_freq_delay < HIGH_OPEN_DELAY_MIN)
            real_delay = HIGH_OPEN_DELAY_MIN * 100;
        else if(g_motor_param.open_freq_delay > HIGH_OPEN_DELAY_MAX)
            real_delay = HIGH_OPEN_DELAY_MAX * 100;
        else 
            real_delay = g_motor_param.open_freq_delay * 100; /*单位0.1秒*/
        g_high_freq.ctrl_delay = real_delay;
    }
    else /*立即关*/
    { 
        g_high_freq.ctrl_delay = 0;
    }
}

unsigned char motor_is_open_freq(void)
{
    return g_high_freq.real_status;
}

static void high_freq_control(void)
{
    if(g_motor_real.current_freq > g_motor_param.open_freq)
    {
        if((g_motor_param.vari_freq_close) && (g_motor_real.freq_arrive == 0)) /*变频关高频*/
        {
            /*关*/
            high_frequery_ctl(2);
        }
        else
        {
            /*开*/
            high_frequery_ctl(1);
        }
    }
    else
    {   /*关*/
        high_frequery_ctl(2);
    }
}


/*外部延时控制时的调用*/
void ext_high_freq_ctl(int period)
{
    uint8_t polarity = 0; /*0:常闭,1:常开*/

    param_get(PARAM0X02, PARAM_HIGH_FREQ_POLARITY, &polarity);
    
    high_freq_control();

    if(g_high_freq.ctrl_delay > period)
        g_high_freq.ctrl_delay -= period;
    else
        g_high_freq.ctrl_delay = 0;

    if(g_high_freq.ctrl_delay == 0)
    {
        if(g_high_freq.ctrl_value == 0)
            return ;
        else if(g_high_freq.ctrl_value == 2){
            bsp_io_ctrl_high_freq(0 , polarity);  //关高频
            ext_send_report_status(0,STATUS_HIGH_FREQ_CHANGE,0);
        }
        else if(g_high_freq.ctrl_value == 1){
            bsp_io_ctrl_high_freq(1 , polarity);  //开高频        
            ext_send_report_status(0,STATUS_HIGH_FREQ_CHANGE,1);
        }
        g_high_freq.real_status = g_high_freq.ctrl_value;
        g_high_freq.ctrl_value = 0;
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

int motor_is_running(void)
{
    if((g_motor_real.motor_status == motor_in_run) || 
        (g_motor_real.motor_status == motor_in_reverse))
        return 1;
    else return 0;
}

/*电机的速度已经稳定，不再变速,1已经稳定，0:还在变速不稳定*/
int motor_speed_is_const(void)  
{
    //return g_motor_real.freq_arrive;
    return g_high_freq.real_status == 1 ? 1 : 0;
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

void motor_brake_start(void)
{
    if((g_motor_real.motor_status == motor_in_brake) ||
       (g_motor_real.motor_status == motor_in_dc_brake))  /*已经在刹车状态*/
        return ;
    g_motor_real.target_freq = g_motor_param.start_freq;
    g_motor_real.motor_status = motor_in_brake;

    high_frequery_ctl(2);
    //EXT_PUMP_DISABLE;
    BREAK_VDC_ENABLE;
}

void motor_dc_brake(void)
{
    bsp_tmr_dc_brake(20);
    g_motor_real.motor_status = motor_in_dc_brake;
    g_motor_real.dc_brake_time = 10 * 1000;   //单位:100us，实际时间1秒钟
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


static unsigned int motor_arrive_freq(float freq)
{
    return float_equal_in_step(g_motor_real.current_freq , freq, 0.01f);
}

void motor_init(void)
{
    bsp_tmr_init();
    g_motor_real.motor_dir = param_dir_load();
    //if(g_motor_real.motor_status != motor_in_idle)
    //    bsp_tmr_start();
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
    //if(radio > RADIO_MAX) /*保护IPM模块*/
    //    radio = RADIO_MAX;

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

/*(T型加减速,下一步频率计算)*/
static float motor_calcu_next_step_freq_t_curve(float current_freq,float target_freq)
{
    /*计算加速时一步的频率和减速时一步的频率,T型加减速用到，如果不是T型，忽略该参数*/
    static float last_target_freq = 0.0f;
    static float acce_step_freq = 0.0f;
    static float dece_step_freq = 0.0f;
    static unsigned int real_acce_time = 0;
    static unsigned int real_dece_time = 0;
    if(float_equal_in_step(last_target_freq , target_freq , 0.01f) == 0)/*重新设置了目标值*/
    {
        /*计算参数*/
        real_acce_time = g_motor_param.acceleration_time_us;
        real_dece_time = g_motor_param.deceleration_time_us;
        if(target_freq <  current_freq){ 
            BREAK_VDC_ENABLE;       /*如果是减速，则打开刹车电阻*/
            unsigned char freq_change = (int)(current_freq - target_freq);
            if((freq_change > 50) && (real_dece_time < 700000))
                real_dece_time = 700000; /*频率变化大于50，最小减速时间0.7秒*/
            else if((freq_change > 40) && (real_dece_time < 400000))
                real_dece_time = 400000;/*频率变化大于40，最小减速时间0.4秒*/
        }
        /*计算减速时间*/
        last_target_freq = target_freq;
        acce_step_freq = ((50.0f - g_motor_param.start_freq) / real_acce_time) * TMR_INT_PERIOD_US;
        dece_step_freq = ((50.0f - g_motor_param.start_freq) / real_dece_time) * TMR_INT_PERIOD_US;
    }

    if(float_equal_in_step(current_freq , target_freq , 0.01f)) {
        BREAK_VDC_DISABLE;      /*关闭刹车电阻*/
        g_motor_real.freq_arrive = 1;
        return target_freq;
    }
    g_motor_real.freq_arrive = 0;
    if(target_freq > current_freq){/*加速*/
        if(float_equal_in_step(current_freq , target_freq, acce_step_freq))
            return target_freq;
        else return current_freq + acce_step_freq;
    } 
    else{/*减速*/
        if(float_equal_in_step(current_freq , target_freq, dece_step_freq))
            return target_freq;
        else return current_freq - dece_step_freq;
    }
}

#if 0
/*三次多项式插值法（Cubic Interpolation） 来生成S型曲线*/
static float motor_calcu_next_step_freq_s_curve(float current_freq,float target_freq)
{
    static float start_freq = 0.0f;
    static float delta_freq = 0.0f;
    static unsigned int total_steps = 0;
    static unsigned int step_count = 0;

    static float last_target_freq = 0.0f;
    if(float_equal_in_step(last_target_freq , target_freq , 0.01f) == 0)
    {
        /*重新计算参数*/
        last_target_freq = target_freq;

        start_freq = current_freq;
        delta_freq = target_freq - current_freq;

        /* 计算动态的加速/减速时间 */
        static float ratio = 0.0f;
        if (target_freq > current_freq) {
            // 加速
            ratio = (target_freq - current_freq) / (50.0f - g_motor_param.start_freq);  // 当前频率比例
            total_steps = (unsigned int)((ratio * g_motor_param.acceleration_time_us) / TMR_INT_PERIOD_US);
        } else {
            // 减速
            ratio = (current_freq - target_freq) / (50.0f - g_motor_param.start_freq);  // 当前频率比例
            total_steps = (unsigned int)((ratio * g_motor_param.deceleration_time_us) / TMR_INT_PERIOD_US);
        }

        if (total_steps == 0) total_steps = 1; // 防止除以零
        step_count = 0;
    }

    /* 如果频率已经到位，直接返回当前频率 */
    if(float_equal_in_step(current_freq , target_freq , 0.01f)) {
        g_motor_real.freq_arrive = 1;
        return target_freq;
    }
    g_motor_real.freq_arrive = 0;
    if (step_count >= total_steps) {
        g_motor_real.freq_arrive = 1;
        return target_freq;
    }

    /*   S 曲线插值：3t² - 2t³  */
    float t = (float)step_count / (float)(total_steps - 1); // [0,1]
    float sigmoid = 3.0f * t * t - 2.0f * t * t * t; // S 型曲线

    float next_freq = start_freq + delta_freq * sigmoid;
    step_count++;

    return next_freq;
}
#endif


#if 0
/**
 * 自定义加减速曲线：前3/4时间完成前半段频率，后1/4时间完成后半段频率
 */
static float motor_calcu_next_step_freq_custom_curve(float current_freq, float target_freq)
{
    static float start_freq = 0.0f;
    static float delta_freq = 0.0f;
    static unsigned int total_steps = 0;
    static unsigned int step_count = 0;

    static float last_target_freq = 0.0f;

    if (!float_equal_in_step(last_target_freq, target_freq, 0.01f))
    {
        /* 重新计算参数 */
        last_target_freq = target_freq;
        start_freq = current_freq;
        delta_freq = target_freq - current_freq;

        /* 根据加速或减速计算总步数 */
        static float ratio = 0.0f;
        if (target_freq > current_freq) {
            // 加速
            ratio = (target_freq - current_freq) / (50.0f - g_motor_param.start_freq);  // 当前频率比例
            total_steps = (unsigned int)((ratio * g_motor_param.acceleration_time_us) / TMR_INT_PERIOD_US);
        } else {
            // 减速
            ratio = (current_freq - target_freq) / (50.0f - g_motor_param.start_freq);  // 当前频率比例
            total_steps = (unsigned int)((ratio * g_motor_param.deceleration_time_us) / TMR_INT_PERIOD_US);
        }

        if (total_steps == 0) total_steps = 1; // 防止除以零
        step_count = 0;
    }

    /* 如果已经到达目标频率 */
    if (float_equal_in_step(current_freq, target_freq, 0.01f))
    {
        g_motor_real.freq_arrive = 1;
        return target_freq;
    }

    g_motor_real.freq_arrive = 0;

    if (step_count >= total_steps)
    {
        g_motor_real.freq_arrive = 1;
        return target_freq;
    }

    float t = (float)step_count / (float)(total_steps - 1); // [0, 1]
    float next_freq;

    /* 分段插值：前半段时间走1/4频率，后半段时间走3/4频率 */
    float s;
    if (t < 0.5f)
    {
        s = t * 0.5f; // 前50%时间走25%频率
    }
    else
    {
        s = 0.25f + (t - 0.5f) * 1.5f; // 后50%时间走75%频率
    }

    next_freq = start_freq + delta_freq * s;
    step_count++;

    return next_freq;
}
#endif

static void motor_update_spwm(void)
{   
    motor_update_compare();

    g_motor_real.current_freq = g_motor_real.next_step_freq;

    /*计算下一步的角度*/
    float delta = PI_2 * TMR_INT_PERIOD_US * g_motor_real.current_freq / 1000000;
    g_motor_real.angle += delta;
    if (g_motor_real.angle >= PI_2) g_motor_real.angle -= PI_2;

    /*计算下一步的频率*/
    g_motor_real.next_step_freq = motor_calcu_next_step_freq_t_curve(g_motor_real.current_freq,g_motor_real.target_freq);
 
}


unsigned int interrupt_times = 0;
 void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
 {
    if(htim->Instance == TIM1)
    {
        interrupt_times++;

        if(g_motor_real.motor_status == motor_in_idle)
            return;

        if(g_motor_real.motor_status == motor_in_dc_brake)
        {
            if(g_motor_real.dc_brake_time != 0)
                g_motor_real.dc_brake_time --;
            else
            {
                /*停机结束*/
                bsp_tmr_stop();
                g_motor_real.motor_status = motor_in_idle;
                high_frequery_ctl(2);
                //EXT_PUMP_DISABLE;
                BREAK_VDC_DISABLE;
                /*保存当前的运行方向（如果是靠边停，没关系）*/
                motor_eeprom_ctl();

            }
            return ;
        }

        if((g_motor_real.motor_status == motor_in_reverse) &&
            (motor_arrive_freq(g_motor_param.start_freq) == 1))
        {
            motor_reverse_recovery();
        }
        else if((g_motor_real.motor_status == motor_in_brake) &&
                (motor_arrive_freq(g_motor_param.start_freq) == 1))
        {
            motor_dc_brake();
        }
        motor_update_spwm(); 
    }
 }