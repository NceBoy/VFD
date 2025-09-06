#include "bsp_key.h"
#include "main.h"
#include "service_data.h"

static uint32_t key_down_count = 0;
static uint8_t  key_send_flag = 0;

void bsp_key_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
 
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);    
}

void bsp_key_detect(void)
{
    if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6) == GPIO_PIN_RESET)
    {
        if(key_down_count < 5000)
        {
            key_down_count += 5;
            if((key_down_count > 1000) && (key_send_flag == 0))
            {
                /*按键连续按下1秒钟*/
                ext_send_notify_to_data(0);
                key_send_flag = 1;
            }
        }
    }
    else
    {
        key_down_count = 0;
        key_send_flag = 0;
    }
}