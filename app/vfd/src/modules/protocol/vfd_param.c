#include "vfd_param.h"
#include "EEPROM.h"


#define EEPROM_ENTRY_SIZE 1  // 每个参数的EEPROM存储大小（字节）

// 定义参数表的存储区域宏
#define PARAM_REGION1_START 0x0000
#define PARAM_REGION2_START (PARAM_REGION1_START + MAX_PARAM_ENTRIES)
#define PARAM_REGION3_START (PARAM_REGION2_START + MAX_PARAM_ENTRIES)

// 全局参数表
uint8_t g_vfdParam[MAX_MODULE_TYPES][MAX_PARAM_ENTRIES];


// 从EEPROM读取所有参数
void pullAllParams() {
    // 从EEPROM的三个区域读取参数
    EEPROM_Read(PARAM_REGION1_START, g_vfdParam[PARAM0X01], MAX_PARAM_ENTRIES);
    EEPROM_Read(PARAM_REGION2_START, g_vfdParam[PARAM0X02], MAX_PARAM_ENTRIES);
    EEPROM_Read(PARAM_REGION3_START, g_vfdParam[PARAM0X03], MAX_PARAM_ENTRIES);
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
    EEPROM_Write(PARAM_REGION1_START, g_vfdParam[PARAM0X01], MAX_PARAM_ENTRIES);
    EEPROM_Write(PARAM_REGION2_START, g_vfdParam[PARAM0X02], MAX_PARAM_ENTRIES);
    EEPROM_Write(PARAM_REGION3_START, g_vfdParam[PARAM0X03], MAX_PARAM_ENTRIES);
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