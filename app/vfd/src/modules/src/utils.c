#include "utils.h"

uint8_t crc8_itu(uint8_t* data, int length)
{
    uint8_t crc = 0x00;
    for (int i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x07; // 多项式0x07
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}


// 计算 Modbus CRC16 校验码
uint16_t crc16_modbus(uint8_t *data, int length) 
{
    uint16_t crc = 0xFFFF; // 初始值
    for (int i = 0; i < length; i++) 
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) 
        {
            if (crc & 0x0001) 
            { // 如果最低位是1
                crc = (crc >> 1) ^ 0xA001; // 右移并异或多项式（0xA001 是 0x8005 的反向）
            } 
            else 
            {
                crc >>= 1; // 否则只右移
            }
        }
    }
    return crc;
}
