#ifndef __INOOUT_H
#define __INOOUT_H


#ifdef __cplusplus
extern "C" {
#endif

#define STATUS_SPEED_CHANGE     (1 << 0)    //0x0001
#define STATUS_PUMP_CHANGE      (1 << 1)    //0x0002
#define STATUS_START_CHANGE     (1 << 2)    //0x0004
#define STATUS_DIRECTION_CHANGE (1 << 3)    //0x0008
#define STATUS_MODE_CHANGE      (1 << 4)    //0x0010
#define STATUS_HIGH_FREQ_CHANGE (1 << 5)    //0x0020


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

/*扫描所有输入输出引脚*/
void inout_scan(void);

/*外部控制的模式信号同步进来*/
void inout_mode_sync_from_ext(unsigned char mode);

/*外部控制的调速信号同步进来*/
void inout_sp_sync_from_ext(unsigned char sp);

/*外部水泵控制，水泵接口扫描使用*/
void ext_ctl_pump(int period);

/*水泵控制，控制水泵时调用该接口*/
void pump_ctl_set_value(int value , int delay);

int pump_ctl_get_value(void);

int pump_ctl_toggle_value(void);

/*电机启动控制*/    
void motor_start_ctl(void);

/*电机停止控制*/
void motor_stop_ctl(stopcode_t code);

/*获取当前调试模式*/
int motor_debug_mode(void);

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