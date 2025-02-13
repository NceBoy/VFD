#ifndef __BSP_LOG_H
#define __BSP_LOG_H

int bsp_log_init(void);

int bsp_log_tx_one(unsigned char c);

int bsp_log_tx(unsigned char* ptr , unsigned int len);


#endif