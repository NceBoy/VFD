#include "main.h"
#include "bsp_io.h"

/*16个输入信号*/
void bsp_io_init_input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
      /* GPIO Ports Clock Enable */

      __HAL_RCC_GPIOA_CLK_ENABLE();
      __HAL_RCC_GPIOB_CLK_ENABLE();
      __HAL_RCC_GPIOC_CLK_ENABLE();
      __HAL_RCC_GPIOD_CLK_ENABLE();    
      
      /*左右限位，超程，结束信号，点动或者4键极性选择*/
      /*Configure GPIO pins : PB3 PB4 PB5 PB10 */  
      GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_10;
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
      /*SP0~SP2，调试，断丝*/
      /*Configure GPIO pins : PC14 PC15 PA1 PC3 PC2 */  
      GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_3|GPIO_PIN_14|GPIO_PIN_15;
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

      
      GPIO_InitStruct.Pin = GPIO_PIN_4;
      HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
      
      
      /*Configure GPIO pins : PD2 PC9 */  /*开丝，开水*/
      GPIO_InitStruct.Pin = GPIO_PIN_2;
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

      GPIO_InitStruct.Pin = GPIO_PIN_9;
      HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
      
      /*Configure GPIO pins : PC8 PB7 */ /*关丝，关水*/
      GPIO_InitStruct.Pin = GPIO_PIN_8;
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
      
      GPIO_InitStruct.Pin = GPIO_PIN_7;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
      
      /*Configure GPIO pins : PC10 PC11 PC12 PC13 */ /*左右限位，超程，加工结束*/
      GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13;
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);     
}


/*2个输出信号，控制继电器*/
void bsp_io_init_output(void)
{

}

