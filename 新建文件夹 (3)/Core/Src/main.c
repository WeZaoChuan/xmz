/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "rtc.h"
#include "sdio.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"   /* 标准输入输出：提供printf等函数 */
#include "mydefine.h" /* 自定义宏定义和函数：项目配置和功能集合 */
#include "string.h"  /* 字符串处理：提供字符串操作函数 */
#include "ff.h"      /* FatFs文件系统核心：提供文件操作API */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* 本节用于声明自定义类型，如结构体、枚举等 */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* 本节用于项目内部的宏定义 */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* 本节用于定义复杂的宏函数 */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* 本节用于声明私有函数原型 */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ===== 信号生成相关参数 ===== */
// 注意：峰值幅度是相对于中心电压 (Vref/2) 的。
//       如果 Vref = 3.3V, 中心电压 = 1.65V。
//       峰值幅度 1000mV 意味着输出电压范围大约在 0.65V 到 2.65V 之间。
uint32_t initial_frequency = 100;      // 初始频率：单位Hz，可通过函数修改
uint16_t initial_peak_amplitude = 1000; // 初始峰值幅度：单位mV，可通过函数修改

/* ===== 显示相关变量 ===== */
 

RTC_TimeTypeDef mytime = {0};
uint32_t hour;
uint32_t min;
uint32_t sec;
RTC_DateTypeDef mydata={0};



/* USER CODE END 0 */



/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* 初始化前的预备工作可以放在这里 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* 其他自定义初始化可以放在这里 */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* 系统级初始化可以放在这里 */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_DAC_Init();
  MX_TIM3_Init();
  MX_TIM6_Init();
  MX_TIM14_Init();
  MX_I2C1_Init();
  MX_SPI2_Init();
  MX_SDIO_SD_Init();
  MX_FATFS_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
  /* ===== 应用层初始化开始 ===== */
  
  /* FFT模块初始化：用于频谱分析 */
  My_FFT_Init();
  
  /* 串口环形缓冲区初始化：用于异步处理串口数据 */
  rt_ringbuffer_init(&uart_ringbuffer, ringbuffer_pool, sizeof(ringbuffer_pool));
  
  /* 信号生成应用初始化 */
  dac_app_init(initial_frequency, initial_peak_amplitude);
  dac_app_set_frequency(1000);
  dac_app_set_waveform(WAVEFORM_TRIANGLE);
  
  /* 按钮应用初始化：处理按键输入 */
  app_btn_init();
  
  /* OLED显示初始化：准备显示硬件 */
  OLED_Init();
  
  /* ADC采样定时器DMA初始化：用于连续信号采集 */
  adc_tim_dma_init();
  
 


 
  
  /* SPI Flash测试（注释掉的部分） */
  //	test_spi_flash();
  
  /* 输出系统初始化完成信息 */
  my_printf(&huart1, "\r\n--- System Init OK ---\r\n");
  
  /* ===== LittleFS文件系统初始化 ===== */
  /* 初始化SPI Flash存储设备 */
  spi_flash_init(); // 确保SPI Flash已就绪
  my_printf(&huart1, "LFS: Initializing storage backend...\r\n");
  /* 初始化LittleFS存储后端 */
  if (lfs_storage_init(&cfg) != LFS_ERR_OK)
  {
    my_printf(&huart1, "LFS: Storage backend init FAILED! Halting.\r\n");
    while (1)
      ; // 存储后端初始化失败时死循环
  }
  my_printf(&huart1, "LFS: Storage backend init OK.\r\n");

  /* 运行LittleFS基本测试：挂载、格式化(如需)和读写测试 */
  lfs_basic_test(); 
  
  /* ===== Shell命令行界面初始化 ===== */
  // 初始化Shell并传入文件系统实例
  shell_init(&lfs);
    
  // 设置UART句柄
  shell_set_uart(&huart1);
  
  /* 初始化任务调度器：管理多任务执行 */
  scheduler_init();
  
  /* 测试SD卡和FAT文件系统功能 */
sd_fatfs_test_all_data_types();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1) /* 主循环：程序将一直在此循环中运行 */
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* 运行任务调度器：处理所有注册的任务 */
    scheduler_run();
		HAL_RTC_GetTime(&hrtc,&mytime,RTC_FORMAT_BIN);
		hour=mytime.Hours;
		min=mytime.Minutes;
		sec=mytime.Seconds;
		HAL_RTC_GetDate(&hrtc, &mydata ,RTC_FORMAT_BIN);
  }
  /* USER CODE END 3 */
}

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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 5;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* 用户代码段4：可添加自定义函数实现 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* 用户可以添加自己的实现来报告HAL错误返回状态 */
  __disable_irq(); /* 禁用中断 */
  while (1) /* 错误处理死循环 */
  {
  }
  /* USER CODE END Error_Handler_Debug */
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
  /* 用户可以添加自己的实现来报告文件名和行号，
     例如: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
