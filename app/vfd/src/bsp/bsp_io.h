#ifndef __BSP_IO_H
#define __BSP_IO_H


#ifdef __cplusplus
extern "C" {
#endif

//水泵控制
#define EXT_PUMP_ENABLE         HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET)
#define EXT_PUMP_DISABLE        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET)
#define EXT_PUMP_TOGGLE         HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_4)

//直流电源控制
#define VFD_VDC_ENABLE          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET)
#define VFD_VDC_DISABLE         HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET)

#if 0
//高频控制
#define HIGH_FREQ_SET           HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET)
#define HIGH_FREQ_RESET         HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET)

//刹车电阻控制
#define BREAK_VDC_ENABLE        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET)
#define BREAK_VDC_DISABLE       HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET)
    
#else
//高频控制
#define HIGH_FREQ_SET        
#define HIGH_FREQ_RESET       

//刹车电阻控制
#define BREAK_VDC_ENABLE        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET)
#define BREAK_VDC_DISABLE       HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET)
 
#endif
void bsp_io_init_input(void);

void bsp_io_init_output(void);

void bsp_ipm_vfo_init(void);

#ifdef __cplusplus
}
#endif

#endif