#ifndef __INOOUT_H
#define __INOOUT_H


#ifdef __cplusplus
extern "C" {
#endif

#define     IO_SCAN_INTERVAL        5
#define     IO_TIMEOUT_MS           5000

typedef enum
{
    CODE_END = 0,                   /*加工结束*/
    CODE_WIRE_BREAK = 1,            /*断丝*/
    CODE_EXCEED = 2,                /*超程*/
    CODE_LIMIT_DOUBLE = 3,          /*左右限位同时触发*/
    CODE_LIMIT_TIMEOUT = 4,         /*左右限位长时间触发*/
    CODE_UNDER_VOLTAGE = 5,         /*欠压*/
    CODE_OVER_VOLTAGE = 6,          /*过压*/
    CODE_POWER_OFF = 7,             /*断电*/
    CODE_IPM_VFO = 8,               /*IPM*/
}stopcode_t;


/*初始化所有输入输出引脚*/
void inout_init(void);

void inout_scan(void);

/*外部控制的模式信号同步进来*/
void inout_mode_sync_from_ext(unsigned char mode);

/*外部控制的调速信号同步进来*/
void inout_sp_sync_from_ext(unsigned char sp);

void pump_ext_ctl(int period);

/*电机启动控制*/
void motor_start_ctl(void);

/*电机停止控制*/
void motor_stop_ctl(stopcode_t code);

/*获取当前速度*/
void inout_get_current_sp(unsigned char* sp , unsigned char* value);

/*获取当前断丝状态*/
unsigned char inout_get_wire_value(void);

/*获取当前限位状态*/
unsigned char inout_get_limit_value(void);

/*获取当前超程状态*/
unsigned char inout_get_exceed_value(void);

/*获取当前errcode*/
unsigned char inout_get_errcode(void);

/*获取当前是否加工结束*/
unsigned char inout_get_work_end(void);

#ifdef __cplusplus
}
#endif

#endif