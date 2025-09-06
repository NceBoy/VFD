#include "main.h"
#include "bsp_uart.h"
#include "log.h"
#include "ringbuffer.h"


static UART_HandleTypeDef huart3;
static ringbuffer_t uart3_rb;
static uint8_t uart3_buffer[1024];
void bsp_uart_init(void)
{
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(&huart3);

    HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8);

    HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8);

    HAL_UARTEx_DisableFifoMode(&huart3);

    __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);

    ringbuffer_init(&uart3_rb, uart3_buffer, sizeof(uart3_buffer));
}


void bsp_uart_send(unsigned char *buf, int len)
{
    HAL_UART_Transmit(&huart3, buf, len, 0xFFFF);
}


void USART3_IRQHandler(void)
{
	HAL_UART_IRQHandler(&huart3);
	
    uint32_t tmp_flag = 0, tmp_it_source = 0;
	tmp_flag = __HAL_UART_GET_FLAG(&huart3, UART_FLAG_RXNE);
	tmp_it_source = __HAL_UART_GET_IT_SOURCE(&huart3, UART_IT_RXNE);
    if((tmp_flag != RESET) && (tmp_it_source != RESET))
    {
		uint8_t data = (uint8_t)(huart3.Instance->RDR & 0xFF);
		ringbuffer_put(&uart3_rb, data);
    }
}

int bsp_uart_recv(unsigned char *buf, int len)
{
    int count = ringbuffer_count(&uart3_rb);
    if(count == 0)
        return 0;
    if(count > len)
        count = len;
    return ringbuffer_read(&uart3_rb, buf, count);
}

uint32_t uart_err = 0;
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
    if(huart->Instance == USART3)
    {
        uart_err++;
    }
}

/**
 * 从Ring Buffer中读取所有可用数据直到达到最大长度限制
 * 该函数会持续读取缓冲区中的数据，直到：
 * 1. 达到指定的最大读取长度(max_len)
 * 2. 缓冲区为空
 * 3. 读取操作失败
 * 
 * 在每次读取后会添加短暂延时，以确保在数据持续写入的场景下
 * 能够读取到更多的数据，避免因检测过快而遗漏数据
 * 
 * @param rb: ring_buffer_t结构体指针
 * @param data: 用于存储读取数据的缓冲区指针
 * @param max_len: 要读取的最大数据长度
 * @return: 实际读取的数据字节数
 */
int bsp_uart_recv_all(unsigned char *data, int max_len) {
    if (!data || max_len == 0) {
        return 0;
    }
    
    size_t read_len = 0;
    size_t available_data;
    
    while (read_len < max_len && (available_data = ringbuffer_count(&uart3_rb)) > 0) {

        size_t to_read = (available_data > (max_len - read_len)) ? 
                         (max_len - read_len) : available_data;
        

        size_t actual_read = ringbuffer_read(&uart3_rb, data + read_len, to_read);
        read_len += actual_read;

        if (actual_read == 0) {
            break;
        }
        HAL_Delay(10);
    }

    return read_len;
}

void bsp_uart_clear(void)
{
    ringbuffer_clear(&uart3_rb);
}