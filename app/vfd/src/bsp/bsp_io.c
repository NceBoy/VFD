#include "main.h"
#include "bsp_io.h"

/*16个输入信号，其中8个开打中断，其余8个扫描模式*/
void bsp_io_init_input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
      /* GPIO Ports Clock Enable */
      __HAL_RCC_GPIOF_CLK_ENABLE();
      __HAL_RCC_GPIOC_CLK_ENABLE();
      __HAL_RCC_GPIOA_CLK_ENABLE();
      __HAL_RCC_GPIOB_CLK_ENABLE();
    
      /*Configure GPIO pins : PC0 PC1 PC2 PC3 */  /*左右限位，超程，结束信号，双边沿中断*/
      GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
      GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
      /*Configure GPIO pins : PA3 PA4 PA5 PA6
                               PA9 PA10 PA11 PA12 */ 
      GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                              |GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
      /*Configure GPIO pins : PB11 PB13 */  /*开丝，开水，常开开关，下降沿中断*/
      GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_13;
      GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
      /*Configure GPIO pins : PB12 PB14 */ /*关丝，关水，常闭开关，上升沿中断*/
      GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_14;
      GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
      /* EXTI interrupt init*/
      HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
      HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    
      HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
      HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    
      HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
      HAL_NVIC_EnableIRQ(EXTI2_IRQn);
    
      HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
      HAL_NVIC_EnableIRQ(EXTI3_IRQn);
    
      HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
      HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}


/*2个输出信号，控制继电器*/
void bsp_io_init_output(void)
{

}

/**
  * @brief This function handles EXTI line0 interrupt.
  */
 void EXTI0_IRQHandler(void)
 {
   HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
 }
 
 /**
   * @brief This function handles EXTI line1 interrupt.
   */
 void EXTI1_IRQHandler(void)
 {
   HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
 }
 
 /**
   * @brief This function handles EXTI line2 interrupt.
   */
 void EXTI2_IRQHandler(void)
 {
   HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
 }
 
 /**
   * @brief This function handles EXTI line3 interrupt.
   */
 void EXTI3_IRQHandler(void)
 {
   HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
 }

 /**
  * @brief This function handles EXTI line[15:10] interrupts.
  */
void EXTI15_10_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
}

