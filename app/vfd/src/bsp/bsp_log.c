#include "stm32g4xx_hal.h"
#include "bsp_log.h"

static UART_HandleTypeDef huart1;

int bsp_log_init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(&huart1);

    HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8);

    HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8);

    HAL_UARTEx_DisableFifoMode(&huart1);

    return 0;
}

int bsp_log_tx_one(unsigned char c)
{
    HAL_UART_Transmit(&huart1, &c, 1, 0xFFFF);
    return 0;
}

int bsp_log_tx(unsigned char* ptr , unsigned int len)
{
    HAL_UART_Transmit(&huart1, ptr, len, 0xFFFF);
    return 0;
}