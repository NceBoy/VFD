#ifndef __MOTOR_H
#define __MOTOR_H


#ifdef __cplusplus
extern "C" {
#endif


void motor_current_freq_set(float freq);
void motor_target_info_update(float target_freq);


void motor_start(float target_freq);
void motor_break(void);
void motor_update_spwm(void);



#ifdef __cplusplus
}
#endif

#endif