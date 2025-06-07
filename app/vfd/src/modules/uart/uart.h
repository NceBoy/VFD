#ifndef __UART_H
#define __UART_H


#ifdef __cplusplus
extern "C" {
#endif

void uart3_init(void);
void uart3_send(unsigned char *buf, int len);
void uart3_period(int period);
void uart3_recv_to_data(void);


#ifdef __cplusplus
}
#endif

#endif