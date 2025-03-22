#ifndef __MOTOR_H
#define __MOTOR_H


#ifdef __cplusplus
extern "C" {
#endif



void motor_target_info_update(float target_freq);
void motor_reverse_start(void);
void motor_break_start(void);


void motor_init(void);
void motor_start(unsigned int dir , float target_freq);



#ifdef __cplusplus
}
#endif

#endif