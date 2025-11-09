#ifndef __MOTOR_H
#define __MOTOR_H


#ifdef __cplusplus
extern "C" {
#endif



void motor_start(unsigned int dir , float target_freq);     /*启动*/
void motor_target_info_update(float target_freq);           /*变速*/
void motor_reverse_start(void);                             /*反向*/
void motor_brake_start(void);                               /*停机*/

int motor_target_current_get(void);
int motor_target_current_dir(void);
int motor_is_working(void);
int motor_is_running(void);
int motor_is_normal_running(void);

void motor_init(void);

void ext_high_freq_ctl(int period);

/*获取当前是否开高频*/
unsigned char motor_is_open_freq(void);


#ifdef __cplusplus
}
#endif

#endif