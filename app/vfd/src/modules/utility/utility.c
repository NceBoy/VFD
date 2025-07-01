#include "utility.h"

uint8_t custom_checksum(uint8_t* data, int length)
{
    unsigned char checksum = 0;
    for (int i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return 0xA5 - checksum;
}


// 计算 Modbus CRC16 校验码
uint16_t modbus_crc16(uint8_t *data, int length) 
{
    uint16_t crc = 0xFFFF; // 初始值
    for (int i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) { // 如果最低位是1
                crc = (crc >> 1) ^ 0xA001; // 右移并异或多项式（0xA001 是 0x8005 的反向）
            } else {
                crc >>= 1; // 否则只右移
            }
        }
    }
    return crc;
}
