/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tx_api.h"
#include "nx_msg.h"
#include "log.h"
#include "bsp_adc.h"
#include "uart.h"
#include "inout.h"
#include "tx_api.h"
#include "tx_initialize.h"
#include "tx_thread.h"
#include "service_test.h"
#include "service_motor.h"
#include "service_data.h"

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);

#define  CFG_TASK_START_PRIO                          2u
#define  CFG_TASK_START_STK_SIZE                    1024u
static  TX_THREAD   taskstarttcb;
static  ULONG64     taskstartstk[CFG_TASK_START_STK_SIZE/8];
static  void        taskstart          (ULONG thread_input);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    tx_kernel_enter();

    /* Infinite loop */
    while (1);
}


VOID  tx_application_define(VOID *first_unused_memory)
{
    tx_thread_create(&taskstarttcb,                 /* 任务控制块地址 */   
                     "task start",                  /* 任务名 */
                     taskstart,                     /* 启动任务函数地址 */
                     0,                             /* 传递给任务的参数 */
                     &taskstartstk[0],              /* 堆栈基地址,8字节对齐 */
                     CFG_TASK_START_STK_SIZE,       /* 堆栈空间大小 */  
                     CFG_TASK_START_PRIO,           /* 任务优先级*/
                     CFG_TASK_START_PRIO,           /* 任务抢占阀值 */
                     TX_NO_TIME_SLICE,              /* 不开启时间片 */
                     TX_AUTO_START);                /* 创建后立即启动 */      
}



static  void  taskstart (ULONG thread_input)
{
	(void)thread_input;

    log_init();

    bsp_adc_init();

	nx_msg_init();

    service_test_start();
    
    service_data_start();

    service_motor_start();
 
 
	while(1)
	{
        inout_scan();
        uart_recv_to_data();
        tx_thread_sleep(5);
	}
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
#if 1 /*170M*/
static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  __HAL_RCC_GPIOF_CLK_ENABLE();
  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV6;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}
#else /*160M*/
 void SystemClock_Config(void)
 {
   RCC_OscInitTypeDef RCC_OscInitStruct = {0};
   RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
 
   /** Configure the main internal regulator output voltage
   */
   HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
 
   /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
   RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
   RCC_OscInitStruct.HSEState = RCC_HSE_ON;
   RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
   RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
   RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV3;
   RCC_OscInitStruct.PLL.PLLN = 40;
   RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
   RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
   RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
   if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
   {
     Error_Handler();
   }
 
   /** Initializes the CPU, AHB and APB buses clocks
   */
   RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                               |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
   RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
   RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
   RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
   RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
 
   if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
   {
     Error_Handler();
   }
 
   /** Enables the Clock Security System
   */
   HAL_RCC_EnableCSS();
 }
#endif


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{

  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }

}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
