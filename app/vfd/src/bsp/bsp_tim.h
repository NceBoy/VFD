#ifndef __BSP_TIM_H
#define __BSP_TIM_H


#ifdef __cplusplus
extern "C" {
#endif

#define SYSCLK_FREQ                      170000000uL
#define TIM_CLOCK_DIVIDER                1
#define PWM_FREQUENCY                    10000
#define ADV_TIM_CLK_MHz                  170 		/* Actual TIM clk including Timer clock divider*/
#define REGULATION_EXECUTION_RATE        1 		/*!< FOC execution rate in number of PWM cycles */
#define PWM_PERIOD_CYCLES     			(uint16_t)(ADV_TIM_CLK_MHz*(uint32_t)1000000u/((uint32_t)(PWM_FREQUENCY)))
#define REP_COUNTER 					(uint16_t) ((REGULATION_EXECUTION_RATE *2u)-1u)
#define HTMIN 							1 /* Required for main.c compilation only, CCR4 is overwritten at runtime */
#define PWM_RESOLUTION 					(uint16_t)(PWM_PERIOD_CYCLES/2)

/*定时器中断周期，多长时间更新一次pwm波*/
#define TMR_INT_PERIOD_US       		(uint16_t)(REGULATION_EXECUTION_RATE*(uint32_t)1000000u/((uint32_t)(PWM_FREQUENCY)))



void bsp_tmr_init(void);

void bsp_tmr_start(void);

void bsp_tmr_stop(void);

void bsp_tmr_dc_brake(unsigned int dc_percent);

void bsp_tmr_update_compare(unsigned short ch1_ccr , unsigned short ch2_ccr , unsigned short ch3_ccr);

#ifdef __cplusplus
}
#endif

#endif