#ifndef __MOTOR_H
#define __MOTOR_H


#ifdef __cplusplus
extern "C" {
#endif


void motor_current_freq_set(float freq);
void motor_target_info_update(float target);


void motor_start(void);
void motor_test(void);


#ifdef __cplusplus
}
#endif

#endif