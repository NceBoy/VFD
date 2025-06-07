#ifndef __BSP_ADC_H
#define __BSP_ADC_H


#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void bsp_adc_init(void);

extern uint32_t u_ima ;
extern uint32_t v_ima ;
extern uint32_t w_ima ;
extern uint32_t vdc_v ;


extern uint32_t adc_u_data ;
extern uint32_t adc_w_data ;
extern uint32_t adc_v_data ;
extern uint32_t adc_data;

#ifdef __cplusplus
}
#endif

#endif