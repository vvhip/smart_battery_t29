/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include <string.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
static void MX_USART1_UART_Init(void);
uint8_t data_val[100] = "BAT001", data_temp[5],data_cap[5],Ccharging[5],Cfull[5];
uint32_t ADC_temp,ADC_cap,charging , is_full ;
int _temp, sum_temp,val_temp;
int _cap, val_cap,sum_cap;

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  // Đây là nơi để người dùng có thể khởi tạo các biến hoặc các cấu hình ban đầu.
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init(); // Khởi tạo HAL Library, reset tất cả các thiết bị ngoại vi và cấu hình hệ thống Systick.

  /* USER CODE BEGIN Init */
  // Khởi tạo các thành phần cần thiết trước khi hệ thống chính bắt đầu.
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config(); // Cấu hình clock của hệ thống.

  /* USER CODE BEGIN SysInit */
  // Khởi tạo các thành phần của hệ thống mà cần thiết trước khi các thiết bị ngoại vi được khởi tạo.
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init(); // Khởi tạo các chân GPIO.
  MX_ADC1_Init(); // Khởi tạo ADC1.
  MX_ADC2_Init(); // Khởi tạo ADC2.
  MX_USART1_UART_Init(); // Khởi tạo UART1.
  /* USER CODE BEGIN 2 */
  // Khởi tạo các thành phần hoặc biến mà người dùng cần sử dụng sau khi tất cả các thiết bị ngoại vi đã được khởi tạo.
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // Chuẩn bị buffer để chứa dữ liệu
    strcpy((char*)data_val, "BAT001");
    strcpy((char*)data_temp, "");
    strcpy((char*)data_cap, "");
    strcpy((char*)Ccharging, "");
    strcpy((char*)Cfull, "");

    // Khởi tạo lại các biến tổng
    sum_cap = 0;
    sum_temp = 0;

    // Đọc ADC 100 lần để lấy giá trị trung bình
    for(int i = 0; i < 100; i++) {
      ADC_cap = 0;
      ADC_temp = 0;
      _temp = 0.0;
      _cap = 0;

      // Bắt đầu chuyển đổi ADC1 và ADC2
      HAL_ADC_Start(&hadc1);
      HAL_ADC_Start(&hadc2);

      // Lấy giá trị từ ADC1 và ADC2
      ADC_temp = HAL_ADC_GetValue(&hadc1);
      ADC_cap = HAL_ADC_GetValue(&hadc2);

      // Dừng chuyển đổi ADC1 và ADC2
      HAL_ADC_Stop(&hadc1);
      HAL_ADC_Stop(&hadc2);

      // Tính toán giá trị dung lượng và nhiệt độ pin từ giá trị ADC
      _cap = ((((float)ADC_cap * 3.3 * 4 / 4096) - 4.2 - 3.0) / 1.2) * 100.0;
      _temp = (float)ADC_temp * 75 / 2048;

      // Cộng dồn các giá trị để tính trung bình
      sum_cap += _cap;
      sum_temp += _temp;

      // Chờ một khoảng thời gian ngắn
      HAL_Delay(10);
    }

    // Đọc trạng thái của các chân GPIO
    charging = 0;
    is_full = 0;

    is_full = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0); // Đọc trạng thái chân GPIO_PIN_0
    charging = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1); // Đọc trạng thái chân GPIO_PIN_1


    // Tính giá trị trung bình của dung lượng và nhiệt độ pin
    val_cap = sum_cap / 100;
    val_temp = sum_temp / 100.0 + 6; // Cộng thêm 6 để điều chỉnh nhiệt độ

    // Chuyển đổi các giá trị thành chuỗi ký tự
    sprintf((char*)data_temp, "%lu", val_temp);
    sprintf((char*)data_cap, "%lu", val_cap);
    sprintf((char*)Ccharging, "%lu", charging);
    sprintf((char*)Cfull, "%lu", is_full);

    // Ghép các chuỗi ký tự lại thành một chuỗi duy nhất
    strcat((char*)data_val, " ");
    strcat((char*)data_val, (char*)data_temp);
    strcat((char*)data_val, " ");
    strcat((char*)data_val, (char*)data_cap);
    strcat((char*)data_val, " ");
    strcat((char*)data_val, (char*)Ccharging);
    strcat((char*)data_val, " ");
    strcat((char*)data_val, (char*)Cfull);
    // strcat((char*)data_val, "\n");

    // Truyền chuỗi ký tự qua UART
    HAL_UART_Transmit(&huart1, data_val, strlen(data_val), 100);

    // Chờ một khoảng thời gian trước khi lặp lại
    HAL_Delay(2000);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_7;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pins : PA1 PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
