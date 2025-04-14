#ifndef PARAM_H
#define PARAM_H

#include <stdint.h>
#include <stdbool.h>
#include "tx_api.h"

//todo
//增加参数表
//增加参数flush，get，put函数


//临时参数
//当前旋转方向，当前工作频率，停机状态 00正常停机 01异常中断

typedef enum {
    SHUTDOWN_DIRECTION_STOP = 0x00,  // 静止
    SHUTDOWN_DIRECTION_LEFT = 0x01,  // 向左
    SHUTDOWN_DIRECTION_RIGHT = 0x02  // 向右
} ShutdownDirection;

typedef enum {
    SHUTDOWN_STATE_NORMAL = 0x00,  // 正常关机
    SHUTDOWN_STATE_EMERGENCY = 0x01  // 异常停机
} ShutdownState;
// 定义结构体
typedef struct {
    uint8_t currentDirection;  // 当前方向
    uint8_t currentFrequency;      // 当前工作频率（单位：Hz）
    uint8_t shutdownState;          // 关机前状态
} DeviceStatus;




typedef enum {
    PARAM0X01 = 0x00,  //
    PARAM0X02 = 0x01,  // 
    PARAM0X03 = 0x02,  // 
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
    PARAM_ACCELERATION_TIME = 0x01,  // 变频加速时间
    PARAM_DECELERATION_TIME = 0x02,  // 变频减速时间
    PARAM_LOW_FREQ_TORQUE_BOOST = 0x03,  // 低频力矩提升
    PARAM_AUTO_ECONOMY_PERCENT = 0x04,  // 自动省电百分比
    PARAM_OVER_VOLTAGE_ADJUST = 0x05,  // 过压调节
    PARAM_MIN_START_FREQ = 0x06  // 最低开高频率
} ModuleParameterType_0x02;

///----------------------------------------------------------------------------//
typedef enum {
    PARAM_HIGH_FREQ_DELAY = 0x01,  // 开高频延时
    PARAM_MAX_POWER_OFF_TIME = 0x02,  // 允许掉电最长时间
    PARAM_STOP_MODE = 0x03,  // 加工停机结束方式
    PARAM_START_FREQ = 0x04,  // 启动、换向、刹车起始频率
    PARAM_WIRE_BREAK_DETECT_TIME = 0x05,  // 断丝检测时间
    PARAM_WIRE_START_DIRECTION = 0x06,  // 丝筒启动方向
    PARAM_PROCESS_END_SIGNAL = 0x07,  // 加工结束信号
    PARAM_WIRE_BREAK_SIGNAL = 0x08  // 断丝检测信号
} ModuleParameterType_0x03;

///----------------------------------------------------------------------------//

typedef enum {
    PARAM_DATA_ALL = 0x00  //全参数下发
} ModuleParameterType_All;

///----------------------------------------------------------------------------//
typedef enum {
    STOP_IMMEDIATE = 0x00,//立即停机
    STOP_ON_RIGHT  = 0x01,//停在右边
    STOP_ON_LEFT  = 0x02,//停在左边
} StopMode;

typedef enum {
    START_MODE_CONTINUE_PREVIOUS_DIRECTION = 0x00,  // 按照之前方向启动
    START_MODE_FORWARD = 0x01,  // 正向启动
    START_MODE_REVERSE = 0x02   // 反向启动
} StartMode;

typedef enum {
    CONTACT_TYPE_CLOSED = 0x00,  // 常闭
    CONTACT_TYPE_OPEN = 0x01     // 常开
} ContactType;


typedef enum {
    FAULT_OVER_VOLTAGE = 0xE01,  // 过压
    FAULT_UNDER_VOLTAGE = 0xE02,  // 低压
    FAULT_WIRE_BREAK = 0xE03,  // 断丝
    FAULT_OVER_TRAVEL = 0xE04,  // 超程
    FAULT_LEFT_RIGHT_SWITCH_BAD = 0xE05,  // 左右开关坏
    FAULT_POWER_OFF = 0xE06,  // 掉电故障
    FAULT_UNKNOWN_07 = 0xE07,  // 未知故障代码（具体含义待定）
    FAULT_UNKNOWN_08 = 0xE08   // 未知故障代码（具体含义待定）
} FaultCode;

#define MAX_MODULE_TYPES 3   // ModuleParameterType数量
#define MAX_PARAM_ENTRIES 16 // 每个模块参数类型的最大参数数量
extern uint8_t g_vfdParam[MAX_MODULE_TYPES][MAX_PARAM_ENTRIES];
// 辅助函数声明
/// 初始化参数表
/// \brief 初始化参数表为默认值。
void initParameterTable();

/// 从EEPROM读取所有参数
/// \brief 从EEPROM的三个区域读取参数。
void pullAllParams();

/// 更新所有参数表
/// \brief 更新所有参数表。
void pushAllParams(uint8_t *newParams);

/// 将参数表完整写入EEPROM
/// \brief 将参数表的三个部分完整写入EEPROM。
void flushAllParams();

/// 获取某维参数
/// \param type 参数类型。
/// \param buffer 用于存储获取的参数的缓冲区。
void pullOneDimension(ModuleParameterType type, uint8_t* buffer);

/// 更新某维参数
/// \param type 参数类型。
/// \param buffer 包含新参数的缓冲区。
void pushOneDimension(ModuleParameterType type, uint8_t* buffer);

/// 将某维参数写入EEPROM
/// \param type 参数类型。
void flushOneDimension(ModuleParameterType type);

/// 获取具体某项参数
/// \param type 参数类型。
/// \param index 参数索引。
/// \param value 存储获取的参数值的变量。
void pullOneItem(ModuleParameterType type, uint8_t index, uint8_t* value);

/// 更新具体某项参数
/// \param type 参数类型。
/// \param index 参数索引。
/// \param value 新的参数值。
void pushOneItem(ModuleParameterType type, uint8_t index, uint8_t value);

/// 将具体某项参数写入EEPROM
/// \param type 参数类型。
/// \param index 参数索引。
void flushOneItem(ModuleParameterType type, uint8_t index);

#endif /* __VFD_PARAM_H__ */