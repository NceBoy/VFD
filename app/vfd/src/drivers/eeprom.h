#ifndef __EEPROM_H
#define __EEPROM_H

#include <stdint.h>
#include <stdbool.h>

// EEPROM 型号选择
#define EEPROM_TYPE_AT24C01        1
#define EEPROM_TYPE_AT24C02        2
#define EEPROM_TYPE_AT24C04        3
#define EEPROM_TYPE_AT24C08        4
#define EEPROM_TYPE_AT24C16        5
#define EEPROM_TYPE_AT24C32        6
#define EEPROM_TYPE_AT24C64        7
#define EEPROM_TYPE_AT24C128       8
#define EEPROM_TYPE_AT24C256       9
#define EEPROM_TYPE_AT24C512       10

// 选择你的型号
#define EEPROM_TYPE    EEPROM_TYPE_AT24C16

// EEPROM I2C 器件地址（7位地址，不带R/W位）
// 标准地址：0xA0 >> 1 = 0x50
#define EEPROM_I2C_ADDR    0xA0

// 自动配置参数
#if   (EEPROM_TYPE == EEPROM_TYPE_AT24C01)
#define EEPROM_SIZE        	128UL
#define EEPROM_PAGE_SIZE   	8
#define EEPROM_ADDR_BYTES   1
#elif (EEPROM_TYPE == EEPROM_TYPE_AT24C02)
#define EEPROM_SIZE        	256UL
#define EEPROM_PAGE_SIZE   	8
#define EEPROM_ADDR_BYTES   1
#elif (EEPROM_TYPE == EEPROM_TYPE_AT24C04)
#define EEPROM_SIZE        	512UL
#define EEPROM_PAGE_SIZE   	16
#define EEPROM_ADDR_BYTES   1
#elif (EEPROM_TYPE == EEPROM_TYPE_AT24C08)
#define EEPROM_SIZE        	1024UL
#define EEPROM_PAGE_SIZE   	16
#define EEPROM_ADDR_BYTES   1
#elif (EEPROM_TYPE == EEPROM_TYPE_AT24C16)
#define EEPROM_SIZE        	2048UL
#define EEPROM_PAGE_SIZE   	16
#define EEPROM_ADDR_BYTES   1
#elif (EEPROM_TYPE == EEPROM_TYPE_AT24C32)
#define EEPROM_SIZE        	4096UL
#define EEPROM_PAGE_SIZE   	32
#define EEPROM_ADDR_BYTES   2
#elif (EEPROM_TYPE == EEPROM_TYPE_AT24C64)
#define EEPROM_SIZE        	8192UL
#define EEPROM_PAGE_SIZE   	32
#define EEPROM_ADDR_BYTES   2
#elif (EEPROM_TYPE == EEPROM_TYPE_AT24C128)
#define EEPROM_SIZE        	16384UL
#define EEPROM_PAGE_SIZE   	64
#define EEPROM_ADDR_BYTES   2
#elif (EEPROM_TYPE == EEPROM_TYPE_AT24C256)
#define EEPROM_SIZE        	32768UL
#define EEPROM_PAGE_SIZE   	64
#define EEPROM_ADDR_BYTES   2
#elif (EEPROM_TYPE == EEPROM_TYPE_AT24C512)
#define EEPROM_SIZE        	65536UL
#define EEPROM_PAGE_SIZE   	128
#define EEPROM_ADDR_BYTES   2
#else
#error "please select your eeprom correctly."
#endif

#define EEPROM_I2C_INDEX        	0x00

#define EEPROM_ERR_NONE             0x00
#define EEPROM_ERR_TIMEOUT          0x01
#define EEPROM_ERR_ARGS             0x02
#define EEPROM_ERR_I2C              0x03

uint8_t eeprom_init(void);
uint8_t eeprom_write(uint32_t addr, const void* buf, uint16_t len);
uint8_t eeprom_read(uint32_t addr, void* buf, uint16_t len);

#endif