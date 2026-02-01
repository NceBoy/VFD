#ifndef __DEBUG_H
#define __DEBUG_H

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN                                                   16

int printk_va(char **out, const char *format, va_list args );

#endif