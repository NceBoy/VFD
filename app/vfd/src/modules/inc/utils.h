#ifndef __UTILS_H__
#define __UTILS_H__ 

#include <stdint.h>

uint8_t crc8_itu(uint8_t* data, int length);

uint16_t crc16_modbus(uint8_t *data, int length) ;


#endif