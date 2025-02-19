#ifndef __MOTOR_H
#define __MOTOR_H


#ifdef __cplusplus
extern "C" {
#endif


void motor_current_freq_set(float freq);
void motor_target_info_update(float target);


void motor_start(float target_freq);
void motor_update_spwm(unsigned int dir , float modulation_ratio);



#ifdef __cplusplus
}
#endif

#endif