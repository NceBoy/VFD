#ifndef __TM1628A_H
#define __TM1628A_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void tm1628a_init(void);
void tm1628a_clear(void);
void tm1628a_set_displaymode(uint8_t mode);
void tm1628a_write_continuous(uint8_t* data , uint8_t length);
void tm1628a_write_once(uint8_t addr , uint8_t data);
void tm1628a_set_brightness(uint8_t level);
void tm1628a_read_key(uint8_t *key);
#ifdef __cplusplus
}
#endif

#endif

