#ifndef __BSP_TMR_H
#define __BSP_TMR_H


#ifdef __cplusplus
extern "C" {
#endif


void bsp_tmr_init(void);

void bsp_tmr_update_compare(unsigned int ch1_ccr , unsigned int ch2_ccr , unsigned int ch3_ccr);

#ifdef __cplusplus
}
#endif

#endif