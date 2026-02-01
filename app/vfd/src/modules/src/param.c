#include "param.h"
#include "eeprom.h"
#include "utils.h"
#include "log.h"
#include <string.h>

uint8_t g_vfdParam[MAX_MODULE_TYPES][MAX_PARAM_ENTRIES];

void param_default(void)
{
    memset(g_vfdParam, 0xff, sizeof(g_vfdParam));
    g_vfdParam[PARAM_TYPE0][PARAM0_FREQ_0] = 50; // 0段频率
    g_vfdParam[PARAM_TYPE0][PARAM0_FREQ_1] = 50; // 1段频率
    g_vfdParam[PARAM_TYPE0][PARAM0_FREQ_2] = 35; // 2段频率
    g_vfdParam[PARAM_TYPE0][PARAM0_FREQ_3] = 20; // 3段频率
    g_vfdParam[PARAM_TYPE0][PARAM0_FREQ_4] = 15; // 4段频率
    g_vfdParam[PARAM_TYPE0][PARAM0_FREQ_5] = 10; // 5段频率
    g_vfdParam[PARAM_TYPE0][PARAM0_FREQ_6] = 50; // 6段频率
    g_vfdParam[PARAM_TYPE0][PARAM0_FREQ_7] = 25; // 7段频率
    g_vfdParam[PARAM_TYPE0][PARAM0_LOW_FREQ] = 15; //  手控盒低速
    g_vfdParam[PARAM_TYPE0][PARAM0_MID_FREQ] = 25; //  手控盒中速
    g_vfdParam[PARAM_TYPE0][PARAM0_HIGH_FREQ] = 35; //  手控盒高速

    g_vfdParam[PARAM_TYPE1][PARAM1_EXCEED_POLARITY] = 0;// 超程极性0：常开 1：常闭
    g_vfdParam[PARAM_TYPE1][PARAM1_LEFT_RIGHT_POLARITY] = 0;// 左右换向信号极性，0:常开 1:常闭
    g_vfdParam[PARAM_TYPE1][PARAM1_HIGH_FREQ_POLARITY] = 0;// 高频极性0：常开 1：常闭
    g_vfdParam[PARAM_TYPE1][PARAM1_STOP_POLARITY] = 2;// 加工结束极性0:常开 1:常闭 2:自适应极性
    g_vfdParam[PARAM_TYPE1][PARAM1_STOP_POSITION] = 0;// 加工停机结束位置0:立即停止 1:靠右停 2:靠左停
    g_vfdParam[PARAM_TYPE1][PARAM1_VARI_FREQ_CLOSE] = 1;// 变频关高频  
    g_vfdParam[PARAM_TYPE1][PARAM1_HIGH_FREQ_DELAY] = 8; // 开高频延时，单位0.1秒
    g_vfdParam[PARAM_TYPE1][PARAM1_BUTTON_TYPE] = 1;// 按键方式0:点动 1:4键

    g_vfdParam[PARAM_TYPE2][PARAM2_ACCE_TIME] = 4;// 变频加速时间，单位0.1秒
    g_vfdParam[PARAM_TYPE2][PARAM2_DECE_TIME] = 4;// 变频减速时间，单位0.1秒
    g_vfdParam[PARAM_TYPE2][PARAM2_HIGH_FREQ] = 8;// 最低开高频频率
    g_vfdParam[PARAM_TYPE2][PARAM2_START_FREQ] = 5;// 启动、换向、刹车起始频率
    g_vfdParam[PARAM_TYPE2][PARAM2_START_DIRECTION] = 0;// 丝筒启动方向，0:之前的方向，1:向左 ， 2:向右
    g_vfdParam[PARAM_TYPE2][PARAM2_TORQUE_BOOST] = 2;// 低频扭矩提升
    g_vfdParam[PARAM_TYPE2][PARAM2_WIRE_BREAK_SIGNAL] = 0;// 断丝检测信号极性0：常开 1：常闭    
    g_vfdParam[PARAM_TYPE2][PARAM2_WIRE_BREAK_TIME] = 2;// 断丝检测时间，单位0.1秒
    g_vfdParam[PARAM_TYPE2][PARAM2_POWER_OFF_TIME] = 0;// 允许掉电最长时间，单位0.1秒
    g_vfdParam[PARAM_TYPE2][PARAM2_AUTO_ECONOMY] = 0;// 自动省电百分比
    g_vfdParam[PARAM_TYPE2][PARAM2_VOLTAGE_ADJUST] = 0;// 过压调节

    g_vfdParam[PARAM_TYPE3][PARAM3_WRITE_PROTECT] = 1;// 数据写保护
    g_vfdParam[PARAM_TYPE3][PARAM3_RECOVERY] = 0;// 数据恢复出厂设置
   
}

 void param_load(void)
 {
    EEPROM_Init();
    EEPROM_Read(0, g_vfdParam, sizeof(g_vfdParam));
    uint8_t crc_param = custom_checksum((uint8_t*)g_vfdParam, sizeof(g_vfdParam) - 1);
    if(g_vfdParam[MAX_MODULE_TYPES - 1][MAX_PARAM_ENTRIES - 1] != crc_param){
        logdbg("param_load: crc error");
        param_default();
    }
 }

 uint8_t param_dir_load(void)
 {
    uint8_t dir = 0;
    EEPROM_Read(PARAM_MOTOR_START_ADDR, &dir, 1);
    if(dir == 0xff){
        dir = 0;
    }
    return dir;
 }

 void param_dir_save(uint8_t dir)
 {
    EEPROM_Write(PARAM_MOTOR_START_ADDR, &dir, 1);
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