#ifndef __BSP_LED_H
#define __BSP_LED_H


#ifdef __cplusplus
extern "C" {
#endif



void bsp_led_init(void);

void bsp_led_run(void);

void bsp_led_ctl(int period);

#ifdef __cplusplus
}
#endif

#endif