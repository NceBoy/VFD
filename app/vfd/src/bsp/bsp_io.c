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
      /*左      :PA11 
        右      :PA12
        超程    :PA15
        结束    :PC10 */
      GPIO_InitStruct.Pin = GPIO_PIN_10;
      HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);     

      GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_15;
      HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  
    
}


/*3个输出信号，控制继电器*/
void bsp_io_init_output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_RESET);
    /*  高频输出开关    :PC3
        水泵开关       :PC4
        变频电源开关    :PC5 */
    GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);  
    
    /*刹车电阻保护接入 PB9*/
    //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
    //GPIO_InitStruct.Pin = GPIO_PIN_9;
    //GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    //GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    //GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    //HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);  
    
    
    //HIGH_FREQ_DISABLE;需要判断高频极性才能控制

    EXT_PUMP_DISABLE;//默认关闭
    VFD_VDC_ENABLE;
    BREAK_VDC_DISABLE;
    
}

void bsp_io_ctrl_pump(int value)
{
    if(value)
    {
        EXT_PUMP_ENABLE;
    }
    else
    {
        EXT_PUMP_DISABLE;
    }
}

/*polarity 0:常开,1:常闭*/
void bsp_io_ctrl_high_freq(int value, int polarity)
{
    if(value)
    {
        if(polarity == 0) /*常开*/
            HIGH_FREQ_SET;
        else
            HIGH_FREQ_RESET;
    }
    else
    {
        if(polarity == 0)
            HIGH_FREQ_RESET;
        else
            HIGH_FREQ_SET;  
    }
}
void bsp_ipm_vfo_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};


    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();   
    
    /*Configure GPIO pin : PC7 */
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
}



