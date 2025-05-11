#include "main.h"
#include "bsp_adc.h"



static ADC_HandleTypeDef hadc1;

uint32_t u_ima = 0;
uint32_t v_ima = 0;
uint32_t w_ima = 0;
uint32_t vdc_v = 0;

void bsp_adc_init(void)
{

    ADC_MultiModeTypeDef multimode = {0};
    ADC_InjectionConfTypeDef sConfigInjected = {0};
    ADC_ChannelConfTypeDef sConfig = {0};
  
    /* USER CODE BEGIN ADC1_Init 1 */
  
    /* USER CODE END ADC1_Init 1 */
  
    /** Common config
    */
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.GainCompensation = 0;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
      Error_Handler();
    }
  
    /** Configure the ADC multi-mode
    */
    multimode.Mode = ADC_MODE_INDEPENDENT;
    if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
    {
      Error_Handler();
    }
  
    /** Configure Injected Channel
    */
    sConfigInjected.InjectedChannel = ADC_CHANNEL_1;
    sConfigInjected.InjectedRank = ADC_INJECTED_RANK_1;
    sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    sConfigInjected.InjectedSingleDiff = ADC_SINGLE_ENDED;
    sConfigInjected.InjectedOffsetNumber = ADC_OFFSET_NONE;
    sConfigInjected.InjectedOffset = 0;
    sConfigInjected.InjectedNbrOfConversion = 3;
    sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
    sConfigInjected.AutoInjectedConv = DISABLE;
    sConfigInjected.QueueInjectedContext = DISABLE;
    sConfigInjected.ExternalTrigInjecConv = ADC_EXTERNALTRIGINJEC_T1_CC4;
    sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONV_EDGE_FALLING;
    sConfigInjected.InjecOversamplingMode = DISABLE;
    if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
    {
      Error_Handler();
    }
  
    /** Configure Injected Channel
    */
    sConfigInjected.InjectedChannel = ADC_CHANNEL_6;
    sConfigInjected.InjectedRank = ADC_INJECTED_RANK_2;
    if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
    {
      Error_Handler();
    }
  
    /** Configure Injected Channel
    */
    sConfigInjected.InjectedChannel = ADC_CHANNEL_7;
    sConfigInjected.InjectedRank = ADC_INJECTED_RANK_3;
    if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
    {
      Error_Handler();
    }
  
    /** Configure Regular Channel
    */
    sConfig.Channel = ADC_CHANNEL_2;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_ADC_ENABLE_IT(&hadc1, ADC_IT_EOC);
    __HAL_ADC_ENABLE_IT(&hadc1, ADC_IT_JEOC);
    HAL_ADCEx_InjectedStart(&hadc1);
}

static void adc_get_current_value(void)
{
    uint32_t adc_u_data = HAL_ADCEx_InjectedGetValue(&hadc1,ADC_INJECTED_RANK_1);
	uint32_t adc_v_data = HAL_ADCEx_InjectedGetValue(&hadc1,ADC_INJECTED_RANK_3);
	uint32_t adc_w_data = HAL_ADCEx_InjectedGetValue(&hadc1,ADC_INJECTED_RANK_2);
	
	u_ima = (uint32_t)(((adc_u_data / 4096.0) * 3.3 / 0.03) * 1000) ;
	v_ima = (uint32_t)(((adc_v_data / 4096.0) * 3.3 / 0.03) * 1000) ;
	w_ima = (uint32_t)(((adc_w_data / 4096.0) * 3.3 / 0.03) * 1000) ;
}

static void adc_get_voltage_value(void)
{
    uint32_t adc_data = HAL_ADC_GetValue(&hadc1);
	
	vdc_v = (uint32_t)(adc_data / 4096.0 * 3.3 * 160 / 1.414);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if(hadc->Instance == ADC1)
    {
        adc_get_voltage_value();
    }
}


void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if(hadc->Instance == ADC1)
    {
        adc_get_current_value();
    }
}

void ADC1_2_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc1);
}

