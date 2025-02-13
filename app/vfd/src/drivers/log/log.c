#include <stdarg.h>
#include "stm32g4xx_hal.h"
#include "printf-stdarg.h"
#include "bsp_log.h"
#include "log.h"

//LOG打印级别
volatile unsigned char log_level = LOG_LEVEL_DBG \
                                    | LOG_LEVEL_INF \
                                    | LOG_LEVEL_WAR \
                                    | LOG_TYPE_ERR;

struct {
    int (*uart_init)(void);
    int (*uart_send_char)(unsigned char);
    int (*uart_send_buf)(unsigned char* ptr , unsigned int len);
    int  (*uart_pop_data)(char* c);
}log_info = {0};

static const struct _str_log_type {
    log_type_t type;
    char *str;
    char *color;
}str_log_type[] = {
  {LOG_TYPE_DBG,    "DBG",  CRT_BLACK},
  {LOG_TYPE_INFO,   "INFO", CRT_BLACK_LIGHT},
  {LOG_TYPE_WAR,    "WAR",  CRT_YELLOW},
  {LOG_TYPE_ERR,    "ERR",  CRT_RED},
  {LOG_TYPE_NONE,   0,      0}
};

/*******************************************************************************
*函数名: log_init
*说明: 打印相关初始化
*参数: 无
*返回: 0 - OK
       -1 - ERROR
其他: 无
*******************************************************************************/
int log_init(void)
{
#if DEBUG_PRINTF_EN > 0
    
    log_info.uart_init = bsp_log_init;
    log_info.uart_send_char = bsp_log_tx_one;
    log_info.uart_send_buf  = bsp_log_tx;
    log_info.uart_pop_data = (void *)0;
    log_info.uart_init();
    
#endif    
    return 0;
}

int log_vprintf(const char *format, va_list args)
{
#if DEBUG_PRINTF_EN > 0
    int ret;

    ret = printk_va(0, format, args );

    return ret;
#else
    return 0;
#endif
}

static int log_printf(const char *format, ...)
{
#if DEBUG_PRINTF_EN > 0
    int ret;
    va_list args;
    
    va_start( args, format );
    
    ret = printk_va(0, format, args );
    
    va_end(args);
    
    return ret;
#else
    return 0;
#endif
}

static struct _str_log_type *str_log_type_get(log_type_t type)
{
    int i;
    
    for(i = 0; 
        (str_log_type[i].type != LOG_TYPE_NONE) \
         && (str_log_type[i].type != type); 
         i ++);
    
    return (struct _str_log_type *)&str_log_type[i];
}

int log_out(const char *format, 
            log_type_t type,
            const char *func,
            int line,
            ...)
{
    int ret;
    unsigned int tick;
    va_list args;
    struct _str_log_type *p_type;
    
    if(type >= LOG_TYPE_NONE)
    {
        return -1;
    }
    
    p_type = str_log_type_get(type);
    if(p_type->str == 0)
    {
        return -1;
    }

    tick = HAL_GetTick();
    log_printf("%s[%s]time:[%010d]%s-%d:\x20",
               p_type->color,
               p_type->str, 
               tick, 
               func, 
               line);
 
    va_start(args, line);
    ret = log_vprintf(format, args);
    va_end(args);
    
    return ret;
}

int log_out_original(const char *format, ...)
{
    
    int ret;
    va_list args;
    
    va_start( args, format );
    
    ret = printk_va(0, format, args );
    
    va_end(args);
    
    return ret;
}

void log_level_set(unsigned char level)
{
    log_level = level;
}

void printchar(char **str, int c)
{
	if (str) {
		**str = c;
		++(*str);
	}
	else
    {
        if(log_info.uart_send_char != (void *)0)
        {
            log_info.uart_send_char(c);
        }
    }
}

void loghex(unsigned char* ptr , unsigned int len)
{
    //log_info.uart_send_buf(ptr , len);
    
    log_out_original("hex[%d]:" , len);
    
    for(int i = 0 ; i < len ; i++)
    {
        log_out_original("%02X ", *(ptr + i));
    }
    
    log_out_original("\n");
    
}
