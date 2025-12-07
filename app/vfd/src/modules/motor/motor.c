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
#include <math.h>
#include <stdbool.h>

#define  HIGH_OPEN_DELAY_MIN      3       /*开高频最小延时，单位0.1秒*/
#define  HIGH_OPEN_DELAY_MAX      30       /*开高频最小延时，单位0.1秒*/

#define ROUND_TO_UINT(x)        ((unsigned int)(x + 0.5))
#define RADIO_MAX               (1.0f)
#define PI                      (3.14159265358979323846f)
#define SQRT3                   (1.73205080756887729353f)
#define INV_SQRT3               (0.57735026918962576451f)  // 1/sqrt(3)

#define MODULATION_MIN    (0.95f)   // 稳态调制比
#define MODULATION_MAX    (1.2f)    // 动态（加速/减速/换向）

#define DEV_BASE_FREQ      (50.0f)
#define DEV_MAX_FREQ       (100.0f)
// 平滑系数（可调）：值越小，响应越慢，越平滑
// 建议范围：0.01 ~ 0.2
#define MOD_SMOOTH_FACTOR (0.05f)

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

/*高频控制:1 开  0关*/
static void high_frequery_ctl(int value)
{
    if(g_high_freq.ctrl_value == value)
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
            high_frequery_ctl(0);
        }
        else
        {
            /*开*/
            high_frequery_ctl(1);
        }
    }
    else
    {   /*关*/
        high_frequery_ctl(0);
    }
}


/*外部延时控制时的调用*/
void ext_high_freq_ctl(int period)
{
    uint8_t should_ctl = 0;
    static uint8_t polarity_last = 0; /*0:常开 1:常闭*/
    uint8_t polarity = 0; 
    param_get(PARAM0X02, PARAM_HIGH_FREQ_POLARITY, &polarity);
    if(polarity != polarity_last){
        should_ctl = 1;
        polarity_last = polarity;
    }
    high_freq_control();
    if(g_high_freq.ctrl_value != g_high_freq.real_status)
        should_ctl = 1;

    if(should_ctl == 0)
        return ;

    if(g_high_freq.ctrl_delay > period)
        g_high_freq.ctrl_delay -= period;
    else
        g_high_freq.ctrl_delay = 0;

    if(g_high_freq.ctrl_delay == 0)
    {
        bsp_io_ctrl_high_freq(g_high_freq.ctrl_value , polarity);  //开关高频        
        g_high_freq.real_status = g_high_freq.ctrl_value;
        ext_send_report_status(0,STATUS_HIGH_FREQ_CHANGE,g_high_freq.real_status);
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

int motor_high_freq_status(void)
{
    return g_high_freq.real_status;
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

    high_frequery_ctl(0);
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
    if(g_motor_real.motor_status != motor_in_idle)
        bsp_tmr_start();
    uint8_t pump = 0, high_freq = 0;
    inout_reset_pump_high_freq(&pump , &high_freq);
    if(pump != 0)
        pump_ctl_set_value(1,0);
    if(high_freq != 0)
        high_frequery_ctl(1);
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


static float torque_boost_from_freq(float freq)
{
    if(freq > 50.0f)
        return 1.0f;
    float torque_boost = 0.0f;

    if(g_motor_param.radio > 6)
        g_motor_param.radio = 6;
    torque_boost = g_radio_rate[g_motor_param.radio];
    return ((1.0f - torque_boost)* freq / 50.0f  + torque_boost);
}
#if 0
static float torque_boost_from_freq(float freq)
{

    if (freq <= DEV_BASE_FREQ) {
        // 基频以下：保留原有转矩提升
        float B = g_radio_rate[MIN(g_motor_param.radio, 6)];
        return (1.0f - B) * (freq / DEV_BASE_FREQ) + B;
    } else {
        // 基频以上：弱磁（1/f 降压）
        if (freq >= DEV_MAX_FREQ) {
            return DEV_BASE_FREQ / DEV_MAX_FREQ; // 最小电压
        }
        return DEV_BASE_FREQ / freq; // 1/f 弱磁
    }
}


static float torque_boost_from_freq(float freq)
{

    if (freq <= DEV_BASE_FREQ) {
        // 基频以下：低频力矩提升
        float B = g_radio_rate[MIN(g_motor_param.radio, 6)];
        return (1.0f - B) * (freq / DEV_BASE_FREQ) + B;
    } else {
        // 基频以上：弱磁区
        float steady_vf = (freq >= DEV_MAX_FREQ) ? (DEV_BASE_FREQ / DEV_MAX_FREQ) : (DEV_BASE_FREQ / freq);

        // === 动态补偿 ===
        if (!motor_speed_is_const()) {
            // 处于加速/减速：尝试提升电压，但不超过当前调制能力
            return 1.0f;  //直接最大调制输出
        }
        return steady_vf;
    }
}

static float torque_boost_from_freq(float freq)
{
    float steady_vf;
    if (freq <= DEV_BASE_FREQ) {
        float B = g_radio_rate[MIN(g_motor_param.radio, 6)];
        steady_vf = (1.0f - B) * (freq / DEV_BASE_FREQ) + B;
    } else {
        if (freq >= DEV_MAX_FREQ) {
            steady_vf = DEV_BASE_FREQ / DEV_MAX_FREQ;
        } else {
            steady_vf = DEV_BASE_FREQ / freq;
        }
    }

    // 全局状态（用于检测状态切换）
    static bool was_in_dynamic = false;
    static float smoothed_tb = 0.0f;

    if (motor_speed_is_const()) {
        // 稳态：更新状态，并直接返回
        was_in_dynamic = false;
        return steady_vf;
    }

    // 动态：检测是否刚进入
    if (!was_in_dynamic) {
        smoothed_tb = steady_vf; // 从当前稳态值开始
    }
    was_in_dynamic = true;

    // 动态目标
    const float DYNAMIC_BOOST_FACTOR = 1.2f;
    const float MAX_DYNAMIC_RATIO = 1.0f;
    float target_tb = MIN(steady_vf * DYNAMIC_BOOST_FACTOR, MAX_DYNAMIC_RATIO);

    // 平滑
    const float SMOOTH_K = 0.2f;
    smoothed_tb += (target_tb - smoothed_tb) * SMOOTH_K;
    if (smoothed_tb > 1.0f) smoothed_tb = 1.0f;
    if (smoothed_tb < 0.0f) smoothed_tb = 0.0f;

    return smoothed_tb;
}
#endif
/**
 * @brief 获取当前平滑后的调制比
 * @return 当前调制比，范围 [MODULATION_MIN, MODULATION_MAX]
 */
float get_smooth_modulation_ratio(void)
{
    static float current_mod = MODULATION_MIN; // 初始为稳态值
    // 设定目标调制比
    float target_mod = motor_speed_is_const() ? MODULATION_MIN : MODULATION_MAX;

    // 一阶低通滤波：平滑过渡
    current_mod += (target_mod - current_mod) * MOD_SMOOTH_FACTOR;

    // 安全限幅（防止浮点误差超限）
    if (current_mod < MODULATION_MIN) current_mod = MODULATION_MIN;
    if (current_mod > MODULATION_MAX) current_mod = MODULATION_MAX;

    return current_mod;
}

// SVPWM 核心计算函数 - 针对中心对齐模式，ARR=8500
static void svpwm_calc_center_aligned(float U_alpha, 
                                      float U_beta, 
                                      float vbus,
                                      unsigned short* Ta, 
                                      unsigned short* Tb, 
                                      unsigned short* Tc)
{
    // 1. 定时器参数设置
    const unsigned short ARR = PWM_RESOLUTION;
    const float U_max = (float)ARR / 2.0f; // 最大电压幅值对应占空比0.5
    // 2. 归一化到 [0, 1] 范围（相对于 PWM 最大值）
    float alpha_norm = U_alpha / U_max;
    float beta_norm  = U_beta  / U_max;

    float U_mag = 0.0f;
    float angle = 0.0f;
    cordic_sqrt_atan2(alpha_norm, beta_norm, &U_mag, &angle);

    // 3. 计算幅值，如果为零则输出中心占空比
    //float U_mag = sqrtf(alpha_norm * alpha_norm + beta_norm * beta_norm);
    if (U_mag > vbus) {
        float scale = vbus / U_mag;
        alpha_norm *= scale;
        beta_norm  *= scale;
        U_mag = vbus;
    }

    // 4. 扇区判断
    //float angle = atan2f(beta_norm, alpha_norm);
    if (angle < 0) angle += 2.0f * PI;

    int sector = (int)(angle / (PI / 3.0f)); // 0 to 5
    if (sector >= 6) sector = 0; // 防止浮点误差导致 sector=6
    float angle_in_sector = angle - sector * (PI / 3.0f);
    // 计算两个相邻矢量的作用时间（归一化到 [0,1]）
    float T1 = U_mag * cordic_sin((PI / 3.0f) - angle_in_sector);
    float T2 = U_mag * cordic_sin(angle_in_sector);
    float T0 = 1.0f - T1 - T2;
    // **过调制处理：如果 T0 < 0，重新分配时间**
    if (T0 < 0) {
        // 过调制区：延长有效矢量时间，T0 = 0
        T1 -= T0 / 2.0f;  // T0 为负，相当于 +|T0|/2
        T2 -= T0 / 2.0f;
        T0 = 0.0f;        // 零矢量时间为 0
    }
    // 三相占空比（中心对齐：占空比 = (T0 + 有效矢量时间)/2）
    float da, db, dc;
    switch(sector) {
        case 0:
            da = T1 + T2 + T0 * 0.5f;
            db = T2 + T0 * 0.5f;
            dc = T0 * 0.5f;
            break;
        case 1:
            da = T1 + T0 * 0.5f;
            db = T1 + T2 + T0 * 0.5f;
            dc = T0 * 0.5f;
            break;
        case 2:
            da = T0 * 0.5f;
            db = T1 + T2 + T0 * 0.5f;
            dc = T1 + T0 * 0.5f;
            break;
        case 3:
            da = T0 * 0.5f;
            db = T2 + T0 * 0.5f;
            dc = T1 + T2 + T0 * 0.5f;
            break;
        case 4:
            da = T1 + T0 * 0.5f;
            db = T0 * 0.5f;
            dc = T1 + T2 + T0 * 0.5f;
            break;
        case 5:
            da = T1 + T2 + T0 * 0.5f;
            db = T0 * 0.5f;
            dc = T2 + T0 * 0.5f;
            break;
        default:
            da = db = dc = 0.5f;
            break;
    }
    // **额外保护：确保占空比在 [0,1] 范围内**
    if (da < 0.0f) da = 0.0f;
    if (da > 1.0f) da = 1.0f;
    if (db < 0.0f) db = 0.0f;
    if (db > 1.0f) db = 1.0f;
    if (dc < 0.0f) dc = 0.0f;
    if (dc > 1.0f) dc = 1.0f;
    // 转换为 CCR（中心对齐模式：CCR = duty * ARR）
    *Ta = (unsigned short)(da * ARR);
    *Tb = (unsigned short)(db * ARR);
    *Tc = (unsigned short)(dc * ARR);

    // 限幅
    if(*Ta > ARR) *Ta = ARR;
    if(*Tb > ARR) *Tb = ARR;
    if(*Tc > ARR) *Tc = ARR;
}

static void motor_update_compare(void)
{
    // 1. 根据当前频率计算电压幅值（V/F + 转矩提升）
    //float modulation = get_smooth_modulation_ratio();  //此时硬件输出的调制系数值
    float modulation = 0.95f;
    float vbus = modulation * INV_SQRT3;//实际值为0.577~0.75
    float torque_boost = torque_boost_from_freq(g_motor_real.current_freq);//范围[0~1.0]
    float Vq_ref = torque_boost * vbus * (PWM_RESOLUTION / 2.0f); // 最大电压幅值对应占空比0.5

    float sin_value = 0.0f;
    float cos_value = 0.0f;
    cordic_sin_cos(g_motor_real.angle, &sin_value, &cos_value);
    // 2. 计算 alpha-beta 轴电压
    //float U_alpha = Vq_ref * cordic_cos(g_motor_real.angle);
    //float U_beta  = Vq_ref * cordic_sin(g_motor_real.angle);
    float U_alpha = Vq_ref * cos_value;
    float U_beta  = Vq_ref * sin_value;
    // 3. 如果反向，交换相序
    if(g_motor_real.motor_dir != 0) {
        U_beta = -U_beta; // 反向时 beta 轴反向
    }

    // 4. SVPWM 核心计算（中心对齐模式）
    unsigned short Ta, Tb, Tc;
    svpwm_calc_center_aligned(U_alpha, U_beta, vbus, &Ta, &Tb, &Tc);

    // 5. 输出到定时器
    bsp_tmr_update_compare(Ta, Tb, Tc);
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
        else return current_freq - acce_step_freq;
    }
}

static void motor_update_spwm(void)
{   
    motor_update_compare();

    g_motor_real.current_freq = g_motor_real.next_step_freq;

    /*计算下一步的角度*/
    float delta = PI * 2.0f * TMR_INT_PERIOD_US * g_motor_real.current_freq / 1000000;
    g_motor_real.angle += delta;
    if (g_motor_real.angle >= PI * 2.0f) g_motor_real.angle -= PI * 2.0f;

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
                high_frequery_ctl(0);
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