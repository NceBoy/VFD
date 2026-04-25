#include "stm32g4xx_hal.h"
#include "bsp_log.h"

static UART_HandleTypeDef hlpuart1;

int bsp_log_init(void)
{
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 921600;
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

    return 0;
}

int bsp_log_tx_one(unsigned char c)
{
    HAL_UART_Transmit(&hlpuart1, &c, 1, 0xFFFF);
    return 0;
}

int bsp_log_tx(unsigned char* ptr , unsigned int len)
{
    HAL_UART_Transmit(&hlpuart1, ptr, len, 0xFFFF);
    return 0;
}