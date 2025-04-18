#ifndef __UART_H
#define __UART_H


#ifdef __cplusplus
extern "C" {
#endif

void lpuart_init(void);
void lpuart_send(unsigned char *buf, int len);
void lpuart_period(int period);
void uart_recv_to_data(void);


#ifdef __cplusplus
}
#endif

#endif