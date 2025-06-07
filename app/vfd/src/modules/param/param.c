#include "param.h"
#include "EEPROM.h"

volatile param_t g_vfdParam = {0};

static void param_default(void)
{

    g_vfdParam.freq[0] = 50; // 0段频率
    g_vfdParam.freq[1] = 50; // 1段频率
    g_vfdParam.freq[2] = 35; // 2段频率
    g_vfdParam.freq[3] = 20; // 3段频率
    g_vfdParam.freq[4] = 15; // 4段频率
    g_vfdParam.freq[5] = 10; // 5段频率
    g_vfdParam.freq[6] = 50; // 6段频率  （高速上丝） 
    g_vfdParam.freq[7] = 25; // 7段频率  （低速上丝） 
    g_vfdParam.freq_low = 15;
    g_vfdParam.freq_med = 15;
    g_vfdParam.freq_high = 15;


    g_vfdParam.acce_time = 8;// 变频加速时间，单位0.1秒
    g_vfdParam.dece_time = 6;// 变频减速时间，单位0.1秒
    g_vfdParam.torque_boost = 2;// 低频力矩提升
    g_vfdParam.auto_economy = 0;// 自动省电百分比
    g_vfdParam.over_voltage = 30;// 过压调节
    g_vfdParam.open_freq = 6;// 最低开高频频率
    g_vfdParam.open_freq_delay = 10;// 开高频延时，单位0.1秒
    g_vfdParam.var_freq_close = 1;// 变频关高频  


    g_vfdParam.start_dir = 1;// 丝筒启动方向
    g_vfdParam.start_freq = 5;// 启动、换向、刹车起始频率
    g_vfdParam.power_off_time = 1;// 允许掉电最长时间，单位0.1秒
    g_vfdParam.work_end = 0;// 加工结束信号
    g_vfdParam.stop_pos = 1;// 加工停机结束方式
    
    g_vfdParam.wire_break = 0;// 断丝检测信号    
    g_vfdParam.wire_break_time = 5;// 断丝检测时间，单位0.1秒
    
}

 void param_init(void)
 {
    EEPROM_Init();
    EEPROM_Read(0, &g_vfdParam, sizeof(g_vfdParam));
    uint8_t crc_param = xor_checksum((uint8_t*)&g_vfdParam, sizeof(g_vfdParam) - 1);
    if(g_vfdParam.crc8 != crc_param)
        param_default();
 }

 void param_save(void)
 {
    g_vfdParam.crc8 = xor_checksum((uint8_t*)&g_vfdParam, sizeof(g_vfdParam) - 1);
    EEPROM_Write(0, &g_vfdParam, sizeof(g_vfdParam));
 }