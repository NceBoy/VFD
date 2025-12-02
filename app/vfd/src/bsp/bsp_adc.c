#include "main.h"
#include "bsp_adc.h"
#include "bsp_io.h"
#include "log.h"

#define THIS_FILE "bsp_adc.c"

#if 0
static ADC_HandleTypeDef hadc1;


uint32_t u_ima = 0;
uint32_t v_ima = 0;
uint32_t w_ima = 0;
uint32_t vdc_v = 0;

void bsp_adc_init(void)
{

    ADC_MultiModeTypeDef multimode = {0};
    ADC_InjectionConfTypeDef sConfigInjected = {0};

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
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler(THIS_FILE, __LINE__);
    }

    /** Configure the ADC multi-mode
     */
    multimode.Mode = ADC_MODE_INDEPENDENT;
    if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
    {
        Error_Handler(THIS_FILE, __LINE__);
    }

    /** Configure Injected Channel
     */
    sConfigInjected.InjectedChannel = ADC_CHANNEL_1;
    sConfigInjected.InjectedRank = ADC_INJECTED_RANK_1;
    sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_24CYCLES_5;
    sConfigInjected.InjectedSingleDiff = ADC_SINGLE_ENDED;
    sConfigInjected.InjectedOffsetNumber = ADC_OFFSET_NONE;
    sConfigInjected.InjectedOffset = 0;
    sConfigInjected.InjectedNbrOfConversion = 4;
    sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
    sConfigInjected.AutoInjectedConv = DISABLE;
    sConfigInjected.QueueInjectedContext = DISABLE;
    sConfigInjected.ExternalTrigInjecConv = ADC_EXTERNALTRIGINJEC_T1_CC4;
    sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONV_EDGE_FALLING;
    sConfigInjected.InjecOversamplingMode = DISABLE;
    if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
    {
        Error_Handler(THIS_FILE, __LINE__);
    }

    /** Configure Injected Channel
     */
    sConfigInjected.InjectedChannel = ADC_CHANNEL_2;
    sConfigInjected.InjectedRank = ADC_INJECTED_RANK_4;
    if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
    {
        Error_Handler(THIS_FILE, __LINE__);
    }

    /** Configure Injected Channel
     */
    sConfigInjected.InjectedChannel = ADC_CHANNEL_6;
    sConfigInjected.InjectedRank = ADC_INJECTED_RANK_3;
    if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
    {
        Error_Handler(THIS_FILE, __LINE__);
    }

    /** Configure Injected Channel
     */
    sConfigInjected.InjectedChannel = ADC_CHANNEL_7;
    sConfigInjected.InjectedRank = ADC_INJECTED_RANK_2;
    if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
    {
        Error_Handler(THIS_FILE, __LINE__);
    }

    __HAL_ADC_ENABLE_IT(&hadc1, ADC_IT_JEOC);
    HAL_ADCEx_InjectedStart(&hadc1);
}

static void adc_get_value(void)
{
    adc_u_data = HAL_ADCEx_InjectedGetValue(&hadc1,ADC_INJECTED_RANK_1);
	adc_v_data = HAL_ADCEx_InjectedGetValue(&hadc1,ADC_INJECTED_RANK_2);
    adc_w_data = HAL_ADCEx_InjectedGetValue(&hadc1,ADC_INJECTED_RANK_3);
	adc_data   = HAL_ADCEx_InjectedGetValue(&hadc1,ADC_INJECTED_RANK_4);

	
    vdc_v = (uint32_t)(adc_data / 4096.0 * 3.3 * 160 / 1.414);
	u_ima = (uint32_t)(((adc_u_data / 4096.0) * 3.3 / 0.03) * 1000) ;
	v_ima = (uint32_t)(((adc_v_data / 4096.0) * 3.3 / 0.03) * 1000) ;
	w_ima = (uint32_t)(((adc_w_data / 4096.0) * 3.3 / 0.03) * 1000) ;
    
}






void ADC1_2_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc1);
    adc_get_value();
}

#endif

#define ADC_CHANNEL_NUM         1
#define ADC_AVERAGE_NUM         2
typedef struct 
{
    uint16_t index;
    uint16_t average;
    uint16_t value[ADC_AVERAGE_NUM];
}adc_data_t;

static ADC_HandleTypeDef hadc1;
static adc_data_t g_adc_value[ADC_CHANNEL_NUM] = {0};
void bsp_adc_init(void)
{

    ADC_MultiModeTypeDef multimode = {0};
    ADC_ChannelConfTypeDef sConfig = {0};

    /* ADC1 Init */
    /** Common config
    */
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.GainCompensation = 0;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = ADC_CHANNEL_NUM;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    hadc1.Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler(THIS_FILE, __LINE__);
    }

    /** Configure the ADC multi-mode
    */
    multimode.Mode = ADC_MODE_INDEPENDENT;
    if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
    {
        Error_Handler(THIS_FILE, __LINE__);
    }

    /** Configure Regular Channel
    */
    sConfig.Channel = ADC_CHANNEL_2;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_247CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler(THIS_FILE, __LINE__);
    }
}


void bsp_adc_start(void)
{
    static uint8_t first_sample = 1;
    if(first_sample)
    {
        HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
        first_sample = 0;
        return;
    }
    uint32_t sum = 0;
    for(int i = 0 ; i < ADC_CHANNEL_NUM ; i++)
    {
        sum = 0;
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 100);
        g_adc_value[i].value[g_adc_value[i].index++] = HAL_ADC_GetValue(&hadc1);
        if(g_adc_value[i].index >= ADC_AVERAGE_NUM)
        {
            g_adc_value[i].index = 0;
            for(int j = 0 ; j < ADC_AVERAGE_NUM ; j++)
                sum += g_adc_value[i].value[j];
            g_adc_value[i].average = sum / ADC_AVERAGE_NUM;            
        }
    }
    HAL_ADC_Stop(&hadc1);
    
    

#if 0    
    int vol = (int)((float)g_adc_value[0].average / 4095.0 * 3 * 160 / 1.414);
    
    if(vol > 270) /*母线电压过高*/
        BREAK_VDC_ENABLE;
    else
        BREAK_VDC_DISABLE;  
#endif
}


int bsp_get_voltage(void)
{
    return (int)((float)g_adc_value[0].average / 4095.0 * 3 * 160 / 1.414);
}

