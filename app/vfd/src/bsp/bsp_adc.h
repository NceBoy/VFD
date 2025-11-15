#ifndef __BSP_ADC_H
#define __BSP_ADC_H


#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void bsp_adc_init(void);

void bsp_adc_start(void);

int bsp_get_voltage(void);

#ifdef __cplusplus
}
#endif

#endif