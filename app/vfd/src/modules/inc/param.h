#ifndef PARAM_H
#define PARAM_H

#include <stdint.h>

#define PARAM_EEPROM_SIZE  2048

#define PARAM_COMMON_START_ADDR     0
#define PARAM_MOTOR_START_ADDR      1024

typedef enum {
    PARAM_TYPE0 = 0x00,  //速度表
    PARAM_TYPE1 = 0x01,  // 用户参数表
    PARAM_TYPE2 = 0x02,  // 厂家参数表
    PARAM_TYPE3 = 0x03,  // 参数密码表
    PARAM_ALL = 0xff,  // 
} ModuleParameterType;
///----------------------------------------------------------------------------//
typedef enum {
    PARAM0_FREQ_0 = 0x00,  // 0段频率
    PARAM0_FREQ_1 = 0x01,  // 1段频率
    PARAM0_FREQ_2 = 0x02,  // 2段频率
    PARAM0_FREQ_3 = 0x03,  // 3段频率
    PARAM0_FREQ_4 = 0x04,  // 4段频率
    PARAM0_FREQ_5 = 0x05,  // 5段频率
    PARAM0_FREQ_6 = 0x06,  // 6段频率
    PARAM0_FREQ_7 = 0x07,  // 7段频率
    PARAM0_LOW_FREQ = 0x08,  // 预设低段频率
    PARAM0_MID_FREQ = 0x09,  // 预设中段频率
    PARAM0_HIGH_FREQ = 0x0A   // 预设高段频率
} ModuleParameterType_0x00;

///----------------------------------------------------------------------------//

typedef enum {
    PARAM1_EXCEED_POLARITY = 0x00,  // 超程极性0：常开 1：常闭
    PARAM1_LEFT_RIGHT_POLARITY = 0x01,  // 左右换向极性0：常开 1：常闭
    PARAM1_HIGH_FREQ_POLARITY = 0x02,  // 高频极性0：常开 1：常闭
    PARAM1_STOP_POLARITY = 0x03 , // 加工结束信号极性0:常开 1:常闭 2:自适应极性
    PARAM1_STOP_POSITION = 0x04,  // 加工停机结束位置0:立即停止 1:靠右停 2:靠左停
    PARAM1_VARI_FREQ_CLOSE = 0x05,  // 变频关高频  
    PARAM1_HIGH_FREQ_DELAY = 0x06,  // 开高频延时
    PARAM1_BUTTON_TYPE = 0x07,  // 按键类型,(0:点动 1:4键)
} ModuleParameterType_0x01;

///----------------------------------------------------------------------------//
typedef enum {
    PARAM2_ACCE_TIME = 0x00,  // 变频加速时间
    PARAM2_DECE_TIME = 0x01,  // 变频减速时间
    PARAM2_HIGH_FREQ = 0x02 , // 最低开高频频率
    PARAM2_START_FREQ = 0x03,  // 启动、换向、刹车起始频率
    PARAM2_START_DIRECTION = 0x04,  // 丝筒启动方向  
    PARAM2_TORQUE_BOOST = 0x05,  // 低频力矩提升  
    PARAM2_WIRE_BREAK_SIGNAL = 0x06,  // 断丝检测信号
    PARAM2_WIRE_BREAK_TIME = 0x07,  // 断丝检测时间
    PARAM2_POWER_OFF_TIME = 0x08,  // 允许掉电最长时间
    PARAM2_AUTO_ECONOMY = 0x09,  // 自动省电百分比
    PARAM2_VOLTAGE_ADJUST = 0x0A,  // 过压调节
} ModuleParameterType_0x02;



typedef enum {
    PARAM3_WRITE_PROTECT = 0x00,     // 数据写保护
    PARAM3_RECOVERY = 0x01,          // 恢复出厂设置
} ModuleParameterType_0x03;

typedef enum {
    FAULT_OVER_VOLTAGE = 0xE01,  // 过压
    FAULT_UNDER_VOLTAGE = 0xE02,  // 低压
    FAULT_WIRE_BREAK = 0xE03,  // 断丝
    FAULT_OVER_TRAVEL = 0xE04,  // 超程
    FAULT_LEFT_RIGHT_SWITCH_BAD = 0xE05,  // 左右开关坏
    FAULT_POWER_OFF = 0xE06,  // 掉电故障
} FaultCode;

#define MAX_MODULE_TYPES    4          // ModuleParameterType数量
#define MAX_PARAM_ENTRIES   12        // 每个模块参数类型的最大参数数量

extern uint8_t g_vfdParam[MAX_MODULE_TYPES][MAX_PARAM_ENTRIES];

void param_load();

void param_default(void);

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

 uint8_t param_dir_load(void);
 void param_dir_save(uint8_t dir);

#endif /* __VFD_PARAM_H__ */