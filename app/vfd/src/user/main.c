#include "main.h"
#include "tx_api.h"
#include "nx_msg.h"
#include "log.h"
#include "bsp_led.h"
#include "bsp_adc.h"
#include "bsp_key.h"
#include "bsp_iwdg.h"
#include "inout.h"
#include "ble.h"
#include "data.h"
#include "motor.h"
#include "param.h"
#include "tx_api.h"
#include "tx_initialize.h"
#include "tx_thread.h"
#include "service_test.h"
#include "service_motor.h"
#include "service_data.h"
#include "service_hmi.h"

#define THIS_FILE       "main.c"

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);

#define  CFG_TASK_START_PRIO                           2u
#define  CFG_TASK_START_STK_SIZE                    1024u
static  TX_THREAD   taskstarttcb;
static  ULONG64     taskstartstk[CFG_TASK_START_STK_SIZE/8];
static  void        taskstart          (ULONG thread_input);

/*电压采样定时器*/
TX_TIMER adc_timer;
static void adc_timer_expire(ULONG id);


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

    bsp_led_init();

    bsp_key_init();

    bsp_iwdg_init();

    inout_init();

	nx_msg_init();

    ble_init();

    param_load();

    //service_test_start();
    
    service_data_start();

    service_motor_start();
 
    service_hmi_start();

    tx_timer_create(&adc_timer , "adc_timer", adc_timer_expire, 0, 20, 10, TX_AUTO_ACTIVATE);
    
    logdbg("build at %s, version %s, system start .\n", __DATE__, VFD_VERSION);

	while(1)
	{
        bsp_iwdg_feed();
        bsp_key_detect(MAIN_CTL_PERIOD);
        data_poll();
        inout_scan();
        bsp_led_run();
        
        inout_pump_ctl(MAIN_CTL_PERIOD);   /*水泵控制*/
        motor_high_freq_ctl(MAIN_CTL_PERIOD); /*高频控制*/
        motor_eeprom_save_check(MAIN_CTL_PERIOD); /*电机结束时保存方向*/
        motor_speed_const_check(MAIN_CTL_PERIOD);
        tx_thread_sleep(MAIN_CTL_PERIOD);
	}
}

static void adc_timer_expire(ULONG id)
{
    (void)id;
    bsp_adc_start();
}

#if 1
/**
  * @brief System Clock Configuration
  * @retval None
  */
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
    Error_Handler(THIS_FILE, __LINE__);
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
    Error_Handler(THIS_FILE, __LINE__);
  }

  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}

#else

/**
  * @brief System Clock Configuration
  * @retval None
  */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler(THIS_FILE, __LINE__);
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
    Error_Handler(THIS_FILE, __LINE__);
  }
}

#endif



/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(const char *file, int line)
{
    logdbg("error occur in: %s line %d, reset.\n", file, line);
    __disable_irq();
    //HAL_NVIC_SystemReset();
    while (1)
    {
    }
}
