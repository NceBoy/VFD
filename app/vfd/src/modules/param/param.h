#ifndef PARAM_H
#define PARAM_H

#include <stdint.h>




typedef enum {
    PARAM0X01 = 0x00,  //
    PARAM0X02 = 0x01,  // 
    PARAM0X03 = 0x02,  // 
    PARAM0X04 = 0x03,
    PARAM_ALL = 0xff,  // 
} ModuleParameterType;
///----------------------------------------------------------------------------//
typedef enum {
    SEGMENT_FREQ_0 = 0x00,  // 0段频率
    SEGMENT_FREQ_1 = 0x01,  // 1段频率
    SEGMENT_FREQ_2 = 0x02,  // 2段频率
    SEGMENT_FREQ_3 = 0x03,  // 3段频率
    SEGMENT_FREQ_4 = 0x04,  // 4段频率
    SEGMENT_FREQ_5 = 0x05,  // 5段频率
    SEGMENT_FREQ_6 = 0x06,  // 6段频率
    SEGMENT_FREQ_7 = 0x07,  // 7段频率
    PRESET_LOW_FREQ = 0x08,  // 预设低段频率
    PRESET_MID_FREQ = 0x09,  // 预设中段频率
    PRESET_HIGH_FREQ = 0x0A   // 预设高段频率
} ModuleParameterType_0x01;

///----------------------------------------------------------------------------//
typedef enum {
    PARAM_ACCE_TIME = 0x00,  // 变频加速时间
    PARAM_DECE_TIME = 0x01,  // 变频减速时间
    PARAM_ORQUE_BOOST = 0x02,  // 低频力矩提升
    PARAM_AUTO_ECONOMY = 0x03,  // 自动省电百分比
    PARAM_VOLTAGE_ADJUST = 0x04,  // 过压调节
    PARAM_HIGH_FREQ = 0x05 , // 最低开高频率
    PARAM_HIGH_FREQ_DELAY = 0x06,  // 开高频延时
    PARAM_VARI_FREQ_CLOSE = 0x07  // 变频关高频   
} ModuleParameterType_0x02;

///----------------------------------------------------------------------------//
typedef enum {
    PARAM_POWER_OFF_TIME = 0x00,  // 允许掉电最长时间
    PARAM_WORK_END_SIGNAL = 0x01,  // 加工结束信号
    PARAM_STOP_MODE = 0x02,  // 加工停机结束方式
    PARAM_START_FREQ = 0x03,  // 启动、换向、刹车起始频率
    PARAM_START_DIRECTION = 0x04,  // 丝筒启动方向
    PARAM_WIRE_BREAK_SIGNAL = 0x05,  // 断丝检测信号
    PARAM_WIRE_BREAK_TIME = 0x06,  // 断丝检测时间
} ModuleParameterType_0x03;

typedef enum {
    PARAM_WRITE_PROTECT = 0x00,  // 数据写保护
    PARAM_RECOVERY = 0x01,  // 恢复出厂设置
} ModuleParameterType_0x04;

typedef enum {
    FAULT_OVER_VOLTAGE = 0xE01,  // 过压
    FAULT_UNDER_VOLTAGE = 0xE02,  // 低压
    FAULT_WIRE_BREAK = 0xE03,  // 断丝
    FAULT_OVER_TRAVEL = 0xE04,  // 超程
    FAULT_LEFT_RIGHT_SWITCH_BAD = 0xE05,  // 左右开关坏
    FAULT_POWER_OFF = 0xE06,  // 掉电故障
} FaultCode;

#define MAX_MODULE_TYPES 4          // ModuleParameterType数量
#define MAX_PARAM_ENTRIES 12        // 每个模块参数类型的最大参数数量

extern uint8_t g_vfdParam[MAX_MODULE_TYPES][MAX_PARAM_ENTRIES];

void param_init();


void param_update_all(uint8_t *newParams);

/// 将参数表完整写入EEPROM
/// \brief 将参数表的三个部分完整写入EEPROM。
void param_save();

/// 获取具体某项参数
/// \param type 参数类型。
/// \param index 参数索引。
/// \param value 存储获取的参数值的变量。
void param_get(ModuleParameterType type, uint8_t index, uint8_t* value);

/// 更新具体某项参数
/// \param type 参数类型。
/// \param index 参数索引。
/// \param value 新的参数值。
void param_set(ModuleParameterType type, uint8_t index, uint8_t value);



#endif /* __VFD_PARAM_H__ */