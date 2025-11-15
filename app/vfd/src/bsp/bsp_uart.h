#ifndef __BSP_UART_H__
#define __BSP_UART_H__ 

#ifdef __cplusplus
extern "C" {
#endif

void bsp_uart_init(void);
void bsp_uart_send(unsigned char *buf, int len);
int bsp_uart_recv(unsigned char *buf, int len);
int bsp_uart_recv_all(unsigned char *data, int max_len);
void bsp_uart_clear(void);


#ifdef __cplusplus
}
#endif


#endif