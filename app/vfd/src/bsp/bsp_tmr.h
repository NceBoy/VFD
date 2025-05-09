#ifndef __BSP_TMR_H
#define __BSP_TMR_H


#ifdef __cplusplus
extern "C" {
#endif


#define PWM_PSC                 0
#define PWM_RESOLUTION          8500
#define PWM_APB2_CLK            170000000

/*中心对称模式下PWM周期，单位微秒*/
#define PWM_CRCLE_IN_US         ((2 * PWM_RESOLUTION * (PWM_PSC + 1) * 1000000) / PWM_APB2_CLK)
#define TMR_RCR                 1

/*定时器中断周期，多长时间更新一次pwm波*/
#define TMR_INT_PERIOD_US       100   /*该值根据rcr和周期计算得出*/




void bsp_tmr_init(void);

void bsp_tmr_start(void);

void bsp_tmr_stop(void);

void bsp_tmr_update_compare(unsigned short ch1_ccr , unsigned short ch2_ccr , unsigned short ch3_ccr);

#ifdef __cplusplus
}
#endif

#endif