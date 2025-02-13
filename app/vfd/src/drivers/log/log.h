#ifndef __LOG_H
#define __LOG_H

#include <stdarg.h>

#define DEBUG_PRINTF_EN                                      1

#define CRT_NONE                                            "\033[0m"
#define CRT_BLACK                                           "\033[0;30m"
#define CRT_BLACK_LIGHT                                     "\033[1;30m"
#define CRT_RED                                             "\033[0;31m"
#define CRT_RED_LIGHT                                       "\033[1;31m"
#define CRT_GREEN                                           "\033[0;32m"
#define CRT_GREEN_LIGHT                                     "\033[1;32m"
#define CRT_YELLOW                                          "\033[1;33m"

#define CRT_CLEAR                                           "\033[2J"

#define ENABLE_FUNC_AND_LINE                                0

int log_init(void);
int log_vprintf(const char *format, va_list args);

extern volatile unsigned char log_level;		/* 设置成变量而不是宏，便于在串口修改调试等级，打印不同的日志信息 */

#define  LOG_LEVEL_OFF            0u   // 全部关闭
#define  LOG_LEVEL_ERR            1u   // 一般错误(Error)
#define  LOG_LEVEL_WAR            2u   // 警告(可能引起错误)(Warn)
#define  LOG_LEVEL_INF            4u   // 运行过程消息(Info)
#define  LOG_LEVEL_DBG            8u   // 调试信息(Debug)

typedef enum {
    LOG_TYPE_DBG = 0,
    LOG_TYPE_INFO,
    LOG_TYPE_WAR,
    LOG_TYPE_ERR,
    LOG_TYPE_NONE,
}log_type_t;

int log_out(const char *format, 
            log_type_t type,
            const char *func,
            int line,
            ...);
int log_out_original(const char *format, ...);
void log_level_set(unsigned char level);
void loghex(unsigned char* ptr , unsigned int len); 

#if ENABLE_FUNC_AND_LINE

#define logdbg(format, ...) \
            do{ \
                if(log_level & LOG_LEVEL_DBG) \
                { \
                    log_out(format, \
                            LOG_TYPE_DBG, \
                            __func__, \
                            __LINE__, ##__VA_ARGS__); \
                } \
            }while(0)
                              
#define loginfo(format, ...) \
            do{ \
                if(log_level & LOG_LEVEL_INF) \
                { \
                    log_out(format, \
                            LOG_TYPE_INFO, \
                            __func__, \
                            __LINE__, ##__VA_ARGS__); \
                } \
            }while(0)
              
#define logwar(format, ...) \
            do{ \
                if(log_level & LOG_LEVEL_WAR) \
                { \
                    log_out(format, \
                            LOG_TYPE_WAR, \
                            __func__, \
                            __LINE__, ##__VA_ARGS__); \
                } \
            }while(0)
              
#define logerr(format, ...) \
            do{ \
                if(log_level & LOG_LEVEL_ERR) \
                { \
                    log_out(format, \
                            LOG_TYPE_ERR, \
                            __func__, \
                            __LINE__, ##__VA_ARGS__); \
                } \
            }while(0)
  
#else
                
#define logdbg(format, ...) \
            do{ \
                if(log_level & LOG_LEVEL_DBG) \
                { \
                    log_out_original(format,##__VA_ARGS__);\
                } \
            }while(0)
                              
#define loginfo(format, ...) \
            do{ \
                if(log_level & LOG_LEVEL_INF) \
                { \
                    log_out_original(format,##__VA_ARGS__);\
                } \
            }while(0)
              
#define logwar(format, ...) \
            do{ \
                if(log_level & LOG_LEVEL_WAR) \
                { \
                    log_out_original(format,##__VA_ARGS__);\
                } \
            }while(0)
              
#define logerr(format, ...) \
            do{ \
                if(log_level & LOG_LEVEL_ERR) \
                { \
                    log_out_original(format,##__VA_ARGS__);\
                } \
            }while(0)
#endif

               
                
#endif
                

