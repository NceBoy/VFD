#ifndef __PARAM_H
#define __PARAM_H


#include "main.h"


typedef struct 
{
    uint8_t freq[8];            //8段速度
    uint8_t freq_low;           //预设低频
    uint8_t freq_med;           //预设中频
    uint8_t freq_high;          //预设高频

    uint8_t acce_time;          // 变频加速时间,单位0.1秒
    uint8_t dece_time;          // 变频减速时间,单位0.1秒
    uint8_t torque_boost;       // 低频力矩提升
    uint8_t auto_economy;       // 自动省电百分比
    uint8_t over_voltage;       // 过压调节
    uint8_t open_freq;          // 最低开高频频率
    uint8_t open_freq_delay;    // 开高频延时,单位0.1秒
    uint8_t var_freq_close;     // 变频关高频 

    uint8_t start_dir;          //启动方向
    uint8_t start_freq;         //启动、换向、刹车起始频率
    uint8_t power_off_time;     // 允许掉电最长时间
    uint8_t work_end;           //加工结束信号极性
    uint8_t stop_pos;           // 加工停机结束方式
    uint8_t wire_break;         //断丝检测信号极性
    uint8_t wire_break_time;    //断丝检测时间

    uint8_t crc8;
}param_t;

 extern volatile param_t g_vfdParam;

 void param_init(void);

 void param_save(void);


#endif /* __VFD_PARAM_H__ */