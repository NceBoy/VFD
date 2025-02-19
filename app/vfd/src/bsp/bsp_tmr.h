#ifndef __BSP_TMR_H
#define __BSP_TMR_H


#ifdef __cplusplus
extern "C" {
#endif


#define PWM_PSC                 16
#define PWM_RESOLUTION          500
#define PWM_APB2_CLK            170000000

/*中心对称模式下PWM周期，单位秒*/
#define PWM_CRCLE               ( (float)(2 * PWM_RESOLUTION * (PWM_PSC + 1)) / PWM_APB2_CLK)

void bsp_tmr_init(void);

void bsp_tmr_start(void);

void bsp_tmr_update_compare(unsigned short ch1_ccr , unsigned short ch2_ccr , unsigned short ch3_ccr);

#ifdef __cplusplus
}
#endif

#endif