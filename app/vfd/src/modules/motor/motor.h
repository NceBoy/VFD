#ifndef __MOTOR_H
#define __MOTOR_H


#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    motor_in_null,
    motor_in_run,
    motor_in_reverse,
    motor_in_break,
}motor_status_enum;

void motor_status_set(motor_status_enum status);
motor_status_enum motor_status_get(void);

float motor_current_freq_get(void);
void motor_target_freq_update(float target_freq);
void motor_reverse(void);
unsigned int motor_arrive_freq(float freq);

void motor_start(unsigned int dir , float target_freq);
void motor_break(void);
void motor_update_spwm(void);



#ifdef __cplusplus
}
#endif

#endif