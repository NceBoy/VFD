#ifndef __UTILITY_H__
#define __UTILITY_H__ 

#include <stdint.h>

uint8_t custom_checksum(uint8_t* data, int length);

uint16_t modbus_crc16(uint8_t *data, int length) ;


#endif