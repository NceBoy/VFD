#include "vfd_param.h"
#include "EEPROM.h"
#include "protocol.h"

#define EEPROM_ENTRY_SIZE 1  // 每个参数的EEPROM存储大小（字节）

// 定义参数表的存储区域宏
#define PARAM_BASE_START    0x0000
#define PARAM_REGION1_START (PARAM_BASE_START)
#define PARAM_REGION2_START (PARAM_REGION1_START + MAX_PARAM_ENTRIES)
#define PARAM_REGION3_START (PARAM_REGION2_START + MAX_PARAM_ENTRIES)
#define PARAM_CHECK_START   (PARAM_REGION3_START + MAX_PARAM_ENTRIES)
// 全局参数表
uint8_t g_vfdParam[MAX_MODULE_TYPES][MAX_PARAM_ENTRIES];


static void param_default(void)
{
    /*表一，共有11个参数*/
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_0] = 50; // 0段频率
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_1] = 50; // 1段频率
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_2] = 35; // 2段频率
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_3] = 20; // 3段频率
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_4] = 15; // 4段频率
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_5] = 10; // 5段频率
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_6] = 50; // 6段频率  （高速上丝） 
    g_vfdParam[PARAM0X01][SEGMENT_FREQ_7] = 25; // 7段频率  （低速上丝） 
    g_vfdParam[PARAM0X01][PRESET_LOW_FREQ] = 15;
    g_vfdParam[PARAM0X01][PRESET_MID_FREQ] = 35;
    g_vfdParam[PARAM0X01][PRESET_HIGH_FREQ] = 50;

    /*表二，共有8个参数*/
    g_vfdParam[PARAM0X02][PARAM_ACCELERATION_TIME] = 8;// 变频加速时间，单位0.1秒
    g_vfdParam[PARAM0X02][PARAM_DECELERATION_TIME] = 6;// 变频减速时间，单位0.1秒
    g_vfdParam[PARAM0X02][PARAM_LOW_FREQ_TORQUE_BOOST] = 2;// 低频力矩提升
    g_vfdParam[PARAM0X02][PARAM_AUTO_ECONOMY_PERCENT] = 0;// 自动省电百分比
    g_vfdParam[PARAM0X02][PARAM_OVER_VOLTAGE_ADJUST] = 30;// 过压调节
    g_vfdParam[PARAM0X02][PARAM_MIN_OPEN_FREQ] = 6;// 最低开高频率
    g_vfdParam[PARAM0X02][PARAM_HIGH_FREQ_DELAY] = 10;// 开高频延时，单位0.1秒
    g_vfdParam[PARAM0X02][PARAM_VARI_FREQ_CLOSE] = 1;// 变频关高频  

    /*表三，共有7个参数*/
    g_vfdParam[PARAM0X03][PARAM_MAX_POWER_OFF_TIME] = 1;// 允许掉电最长时间
    g_vfdParam[PARAM0X03][PARAM_STOP_MODE] = 1;// 加工停机结束方式
    g_vfdParam[PARAM0X03][PARAM_START_FREQ] = 5;// 启动、换向、刹车起始频率
    g_vfdParam[PARAM0X03][PARAM_WIRE_BREAK_DETECT_TIME] = 0;// 断丝检测时间，单位0.1秒
    g_vfdParam[PARAM0X03][PARAM_WIRE_START_DIRECTION] = 1;// 丝筒启动方向
    g_vfdParam[PARAM0X03][PARAM_PROCESS_END_SIGNAL] = 0;// 加工结束信号
    g_vfdParam[PARAM0X03][PARAM_WIRE_BREAK_SIGNAL] = 0;// 断丝检测信号

}

// 从EEPROM读取所有参数
void pullAllParams() {
    // 从EEPROM的三个区域读取参数
    //EEPROM_Read(PARAM_REGION1_START, g_vfdParam[PARAM0X01], MAX_PARAM_ENTRIES);
    //EEPROM_Read(PARAM_REGION2_START, g_vfdParam[PARAM0X02], MAX_PARAM_ENTRIES);
    //EEPROM_Read(PARAM_REGION3_START, g_vfdParam[PARAM0X03], MAX_PARAM_ENTRIES);
    EEPROM_Read(PARAM_BASE_START, g_vfdParam, sizeof(g_vfdParam));

    uint16_t crc = 0;
    EEPROM_Read(PARAM_CHECK_START, &crc, sizeof(crc));
    uint16_t crc_param = calculate_modbus_crc(g_vfdParam, sizeof(g_vfdParam));
    if(crc != crc_param)
        param_default();
}

// 更新所有参数表
void pushAllParams(uint8_t *newParams) {
    if (newParams != NULL) {
        memcpy(g_vfdParam, newParams, sizeof(g_vfdParam));
    }
}
// 将参数表完整写入EEPROM
void flushAllParams() {
    // 将参数表的三个部分写入EEPROM
    //EEPROM_Write(PARAM_REGION1_START, g_vfdParam[PARAM0X01], MAX_PARAM_ENTRIES);
    //EEPROM_Write(PARAM_REGION2_START, g_vfdParam[PARAM0X02], MAX_PARAM_ENTRIES);
    //EEPROM_Write(PARAM_REGION3_START, g_vfdParam[PARAM0X03], MAX_PARAM_ENTRIES);

    EEPROM_Write(PARAM_BASE_START, g_vfdParam, sizeof(g_vfdParam));
    uint16_t crc_param = calculate_modbus_crc(g_vfdParam, sizeof(g_vfdParam));
    EEPROM_Write(PARAM_CHECK_START, &crc_param, sizeof(crc_param));
}

// 获取某维参数
void pullOneDimension(ModuleParameterType type, uint8_t* buffer) {
    if (type < MAX_MODULE_TYPES) {
        memcpy(buffer, g_vfdParam[type], MAX_PARAM_ENTRIES);
    }
}

// 更新某维参数
void pushOneDimension(ModuleParameterType type, uint8_t* buffer) {
    if (type < MAX_MODULE_TYPES) {
        memcpy(g_vfdParam[type], buffer, MAX_PARAM_ENTRIES);
    }
}

// 将某维参数写入EEPROM
void flushOneDimension(ModuleParameterType type) {
    uint32_t regionStart;
    switch (type) {
        case PARAM0X01:
            regionStart = PARAM_REGION1_START;
            break;
        case PARAM0X02:
            regionStart = PARAM_REGION2_START;
            break;
        case PARAM0X03:
            regionStart = PARAM_REGION3_START;
            break;
        default:
            return; // 无效的ModuleParameterType
    }
    EEPROM_Write(regionStart, g_vfdParam[type], MAX_PARAM_ENTRIES);
}

// 获取具体某项参数
void pullOneItem(ModuleParameterType type, uint8_t index, uint8_t* value) {
    if (type < MAX_MODULE_TYPES && index < MAX_PARAM_ENTRIES) {
        *value = g_vfdParam[type][index];
    }
}

// 更新具体某项参数
void pushOneItem(ModuleParameterType type, uint8_t index, uint8_t value) {
    if (type < MAX_MODULE_TYPES && index < MAX_PARAM_ENTRIES) {
        g_vfdParam[type][index] = value;
    }
}

// 将具体某项参数写入EEPROM
void flushOneItem(ModuleParameterType type, uint8_t index) {
    uint32_t regionStart;
    uint16_t offset;
    switch (type) {
        case PARAM0X01:
            regionStart = PARAM_REGION1_START;
            break;
        case PARAM0X02:
            regionStart = PARAM_REGION2_START;
            break;
        case PARAM0X03:
            regionStart = PARAM_REGION3_START;
            break;
        default:
            return; // 无效的ModuleParameterType
    }
    offset = index;
    EEPROM_Write(regionStart + offset, &g_vfdParam[type][index], EEPROM_ENTRY_SIZE);
}

// 初始化参数表
void initParameterTable() {
    uint8_t error;
    
    // 初始化EEPROM
    error = EEPROM_Init();
    if (error != EEPROM_ERR_NONE) {
        // EEPROM初始化失败，可能需要恢复默认值或等待重试
        //printf("EEPROM initialization failed: error code %d\n", error);
        return;
    }

    // 从EEPROM读取所有参数
    pullAllParams();
}