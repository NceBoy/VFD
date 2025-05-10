#include "main.h"
#include "bsp_io.h"

/*17个输入信号*/
void bsp_io_init_input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
      /* GPIO Ports Clock Enable */

      __HAL_RCC_GPIOA_CLK_ENABLE();
      __HAL_RCC_GPIOB_CLK_ENABLE();
      __HAL_RCC_GPIOC_CLK_ENABLE();
      __HAL_RCC_GPIOD_CLK_ENABLE();    
      
      /*左右限位，超程，结束信号，点动或者4键极性选择*/
      /*左右限位极性 :PB15 
        结束极性    :PB14 
        超程极性    :PB13 
        点动四键    :PB12 */  
      GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
      /*SP0~SP2，调试，断丝*/
      /*SP0 : PD2
        SP1 : PC12
        SP2 : PC11
        断丝 : PC13
        调试 : PB7 */  
      GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13;
      HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

      GPIO_InitStruct.Pin = GPIO_PIN_2;
      HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
      
      GPIO_InitStruct.Pin = GPIO_PIN_7;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
      
      /*开丝，关丝，开水，关水*/
      /*开丝    :PB3 
        关丝    :PB4 
        开水    :PB5 
        关水    :PB6 */
      GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

      /*左右限位，超程，加工结束*/
      /*左    :PA11 
        右    :PA12
        超程    :PA15
        结束    :PC10 */
      GPIO_InitStruct.Pin = GPIO_PIN_10;
      HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);     

      GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_15;
      HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  
}


/*2个输出信号，控制继电器*/
void bsp_io_init_output(void)
{
    /*水泵控制*/


    /*高频控制*/


}

