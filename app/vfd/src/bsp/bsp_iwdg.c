#include "bsp_iwdg.h"
#include "main.h"

static IWDG_HandleTypeDef hiwdg;
void bsp_iwdg_init(void)
{
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_32; /*32KHz/32=1KHz*/
  hiwdg.Init.Window = 2000;
  hiwdg.Init.Reload = 2000;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
}

void bsp_iwdg_feed(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}