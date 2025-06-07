#ifndef __INOOUT_H
#define __INOOUT_H


#ifdef __cplusplus
extern "C" {
#endif

#define     IO_SCAN_INTERVAL        5
#define     IO_TIMEOUT_MS           5000



/*初始化所有输入输出引脚*/
void inout_init(void);

void inout_scan(void);

/*外部控制的调速信号同步进来*/
void inout_sp_sync_from_ext(unsigned char sp);

void motor_start_ctl(void);

void motor_stop_ctl(void);



#ifdef __cplusplus
}
#endif

#endif