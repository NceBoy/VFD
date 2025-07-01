#ifndef __SERVICE_MOTOR_H
#define __SERVICE_MOTOR_H

#ifdef __cplusplus
 extern "C" {
#endif



void service_motor_start(void);

void ext_motor_start(unsigned int dir , unsigned int target);
void ext_motor_speed(unsigned int speed);  
void ext_motor_reverse(void);
void ext_motor_brake(void);

#ifdef __cplusplus
}
#endif

#endif