#include "param.h"
#include "EEPROM.h"
#include "utility.h"
#include <string.h>

uint8_t g_vfdParam[MAX_MODULE_TYPES][MAX_PARAM_ENTRIES];

void param_default(void)
{
    memset(g_vfdParam, 0xff, sizeof(g_vfdParam));
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_0] = 50; // 0段频率
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_1] = 50; // 1段频率
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_2] = 35; // 2段频率
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_3] = 20; // 3段频率
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_4] = 15; // 4段频率
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_5] = 10; // 5段频率
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_6] = 50; // 6段频率(高速上丝)
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_7] = 25; // 7段频率(低速上丝)
    g_vfdParam[PARAM0X01][PRESET_LOW_FREQ] = 15; //  手控盒低速
    g_vfdParam[PARAM0X01][PRESET_MID_FREQ] = 25; //  手控盒中速
    g_vfdParam[PARAM0X01][PRESET_HIGH_FREQ] = 35; //  手控盒高速

    g_vfdParam[PARAM0X02][PARAM_ACCE_TIME] = 6;// 变频加速时间，单位0.1秒
    g_vfdParam[PARAM0X02][PARAM_DECE_TIME] = 6;// 变频减速时间，单位0.1秒
    g_vfdParam[PARAM0X02][PARAM_ORQUE_BOOST] = 2;// 低频力矩提升
    g_vfdParam[PARAM0X02][PARAM_AUTO_ECONOMY] = 0;// 自动省电百分比
    g_vfdParam[PARAM0X02][PARAM_VOLTAGE_ADJUST] = 30;// 过压调节
    g_vfdParam[PARAM0X02][PARAM_HIGH_FREQ] = 6;// 最低开高频频率
    g_vfdParam[PARAM0X02][PARAM_HIGH_FREQ_DELAY] = 10;// 开高频延时，单位0.1秒
    g_vfdParam[PARAM0X02][PARAM_VARI_FREQ_CLOSE] = 1;// 变频关高频  
    g_vfdParam[PARAM0X02][PARAM_HIGH_FREQ_POLARITY] = 0; // 开高频极性，0:常开 1:常闭

    g_vfdParam[PARAM0X03][PARAM_POWER_OFF_TIME] = 1;// 允许掉电最长时间，单位0.1秒
    g_vfdParam[PARAM0X03][PARAM_WORK_END_SIGNAL] = 0;// 加工结束信号极性
    g_vfdParam[PARAM0X03][PARAM_STOP_MODE] = 0;// 加工停机结束方式0:立即停止 1:靠右停 2:靠左停
    g_vfdParam[PARAM0X03][PARAM_START_FREQ] = 5;// 启动、换向、刹车起始频率
    g_vfdParam[PARAM0X03][PARAM_WIRE_BREAK_SIGNAL] = 0;// 断丝检测信号极性    
    g_vfdParam[PARAM0X03][PARAM_WIRE_BREAK_TIME] = 5;// 断丝检测时间，单位0.1秒
    g_vfdParam[PARAM0X03][PARAM_START_DIRECTION] = 1;// 丝筒启动方向
    g_vfdParam[PARAM0X03][PARAM_LEFT_RIGHT_SIGNAL] = 0;// 左右换向信号极性
    g_vfdParam[PARAM0X03][PARAM_EXCEED_SIGNAL] = 0;// 超程信号极性
    g_vfdParam[PARAM0X03][PARAM_JOY_KEY_SIGNAL] = 0;// 点动还是4键(0:点动 1:4键)


    g_vfdParam[PARAM0X04][PARAM_WRITE_PROTECT] = 1;// 数据写保护
    g_vfdParam[PARAM0X04][PARAM_RECOVERY] = 0;// 数据恢复出厂设置
   
}

 void param_load(void)
 {
    EEPROM_Init();
    EEPROM_Read(0, g_vfdParam, sizeof(g_vfdParam));
    uint8_t crc_param = custom_checksum((uint8_t*)g_vfdParam, sizeof(g_vfdParam) - 1);
    if(g_vfdParam[MAX_MODULE_TYPES - 1][MAX_PARAM_ENTRIES - 1] != crc_param)
        param_default();
 }

 void param_save(void)
 {
    uint8_t crc_param = custom_checksum((uint8_t*)g_vfdParam, sizeof(g_vfdParam) - 1);
    g_vfdParam[MAX_MODULE_TYPES - 1][MAX_PARAM_ENTRIES - 1] = crc_param;
    EEPROM_Write(0, g_vfdParam, sizeof(g_vfdParam));
 }

 void param_update_all(uint8_t *newParams)
 {
    memcpy(g_vfdParam, newParams, sizeof(g_vfdParam) - 1);
    param_save();
 }

 void param_get(ModuleParameterType type, uint8_t index, uint8_t* value)
 {
    if (type < MAX_MODULE_TYPES && index < MAX_PARAM_ENTRIES) {
        *value = g_vfdParam[type][index];
    }
 }

 void param_set(ModuleParameterType type, uint8_t index, uint8_t value)
 {
    if (type < MAX_MODULE_TYPES && index < MAX_PARAM_ENTRIES) {
        g_vfdParam[type][index] = value;
    }
 }