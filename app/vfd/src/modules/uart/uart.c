#include "main.h"
#include "uart.h"
#include "service_data.h"
#include <stdint.h>

typedef enum
{
	UART_RECV_IDLE = 0,
	UART_RECV_ING,
    UART_RECV_FINISH,
}uart_recv_state_t;

typedef struct
{
	uint8_t*            recv_buf;				//接收数据缓冲区首地址
	uint16_t            recv_len;				//接收数据长度
	uint16_t            recv_last;			    //距离上次接收数据的时间
    uint32_t            recv_buf_max_len;       //接收数据缓冲区最大长度
    uint32_t            recv_timeout_ms;        //接收超时
	uart_recv_state_t   recv_state;		        //接收状态标志
}uart_recv_t;

#define LP_UART_RECV_BUF_LEN          128     /*串口接收缓冲区大小*/

uint8_t lpuart_recv_buf[LP_UART_RECV_BUF_LEN];
uart_recv_t lpuart_param;
static UART_HandleTypeDef hlpuart1;


static void uart_para_init(uart_recv_t *param , uint8_t* recv_buf , uint32_t recv_buf_max_len , uint32_t recv_timeout_ms)
{
    param->recv_buf = recv_buf;
	param->recv_last = 0;
	param->recv_len = 0;
    param->recv_buf_max_len = recv_buf_max_len;
    param->recv_timeout_ms = recv_timeout_ms;
	param->recv_state = UART_RECV_IDLE;
}

static void uart_in_data(uart_recv_t *param , uint8_t data)
{
    if(param->recv_state == UART_RECV_IDLE)
    {
        param->recv_state = UART_RECV_ING;
        param->recv_last = 0;
        param->recv_len = 0;
        param->recv_buf[param->recv_len++] = data;
        
    }
    else if(param->recv_state == UART_RECV_ING)
    {
        param->recv_last = 0;
        if(param->recv_len < param->recv_buf_max_len)
            param->recv_buf[param->recv_len++] = data;
    }
}

/*必须保证period_ms调用一次，不然超时时间会出错
 如: 1ms 调用一次，period_ms = 1
     5ms 调用一次，period_ms = 5
*/
static void uart_recv_status(uart_recv_t *param , uint32_t period_ms)
{
	if(param->recv_last < param->recv_timeout_ms)
		param->recv_last += period_ms;
	else
	{
		if(param->recv_state == UART_RECV_ING)
		{
            /*发送数据至消息处理接口*/
            //ext_send_to_data(1 , param->recv_buf , param->recv_len);
			param->recv_state = UART_RECV_FINISH;
		}
	}      
}


void uart_recv_to_data(void)
{
    if(lpuart_param.recv_state == UART_RECV_FINISH)
    {
        ext_send_to_data(1 , lpuart_param.recv_buf , lpuart_param.recv_len);
        lpuart_param.recv_state = UART_RECV_IDLE;
    }
        
}

void lpuart_init(void)
{
    hlpuart1.Instance = LPUART1;
    hlpuart1.Init.BaudRate = 115200;
    hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits = UART_STOPBITS_1;
    hlpuart1.Init.Parity = UART_PARITY_NONE;
    hlpuart1.Init.Mode = UART_MODE_TX_RX;
    hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(&hlpuart1);

    HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8);

    HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8);

    HAL_UARTEx_DisableFifoMode(&hlpuart1);

    uart_para_init(&lpuart_param,lpuart_recv_buf,LP_UART_RECV_BUF_LEN , 50);
    __HAL_UART_ENABLE_IT(&hlpuart1, UART_IT_RXNE);
}

void lpuart_period(int period_ms)
{
    /*LPUART接收状态处理*/
    uart_recv_status(&lpuart_param , period_ms);
}



void lpuart_send(unsigned char *buf, int len)
{
    HAL_UART_Transmit(&hlpuart1, buf, len, 0xFFFF);
}


void LPUART1_IRQHandler(void)
{
	HAL_UART_IRQHandler(&hlpuart1);
	
    uint32_t tmp_flag = 0, tmp_it_source = 0;
	tmp_flag = __HAL_UART_GET_FLAG(&hlpuart1, UART_FLAG_RXNE);
	tmp_it_source = __HAL_UART_GET_IT_SOURCE(&hlpuart1, UART_IT_RXNE);
    if((tmp_flag != RESET) && (tmp_it_source != RESET))
    {
		uint8_t data = (uint8_t)(hlpuart1.Instance->RDR & 0xFF);
		uart_in_data(&lpuart_param,data);
    }
}

uint32_t lpuart_err = 0;
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    uint32_t err = HAL_UART_GetError(huart);
	if(err &(HAL_UART_ERROR_PE | HAL_UART_ERROR_NE | HAL_UART_ERROR_FE | HAL_UART_ERROR_ORE))
    {
        if(err & HAL_UART_ERROR_ORE) /*溢出错误会关闭接收，此时重新打开*/
        {
            huart->Instance->CR1 |= UART_IT_RXNE;
        }  
        huart->ErrorCode = HAL_UART_ERROR_NONE;
    }
    if(huart->Instance == LPUART1)
    {
        lpuart_err++;
    }
}