#ifndef __BSP_IO_H
#define __BSP_IO_H


#ifdef __cplusplus
extern "C" {
#endif

#define HIGH_FREQ_ENABLE        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET)
#define HIGH_FREQ_DISABLE       HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET)

#define EXT_PUMP_ENABLE         HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET)
#define EXT_PUMP_DISABLE        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET)
#define EXT_PUMP_TOGGLE         HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_4)

#define VFD_VDC_ENABLE          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET)
#define VFD_VDC_DISABLE         HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET)

void bsp_io_init_input(void);

void bsp_io_init_output(void);

void bsp_ipm_vfo_init(void);

#ifdef __cplusplus
}
#endif

#endif