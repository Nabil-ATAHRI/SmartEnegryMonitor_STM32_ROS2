/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "pzem004t.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define POLL_INTERVAL 1000U
#define MAX_RETRIES 3U
#define STARTUP_ATTEMPTS 5U
#define UART4_LOOPBACK_TEST 0U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart4;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_UART4_Init(void);
/* USER CODE BEGIN PFP */
PZEM_t pzem;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_putchar(int ch)
{
  uint8_t byte = (uint8_t)ch;
  HAL_UART_Transmit(&huart2, &byte, 1, HAL_MAX_DELAY);
  return ch;
}

static void SendPzemToRaspberry(bool valid, float voltage, float current,
                                float power, float energy, float frequency,
                                float powerFactor)
{
  char line[96];
  int length;

  if (valid) {
    length = snprintf(line, sizeof(line),
                      "PZEM,OK,%.1f,%.3f,%.1f,%.3f,%.1f,%.2f\r\n",
                      voltage, current, power, energy, frequency, powerFactor);
  } else {
    length = snprintf(line, sizeof(line), "PZEM,ERROR,0,0,0,0,0,0\r\n");
  }

  HAL_StatusTypeDef txStatus = HAL_ERROR;
  if (length > 0 && length < (int)sizeof(line)) {
    txStatus = HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)length, 100);
  }

  if (txStatus == HAL_OK) {
    printf("USART1 -> Raspberry Pi: %s record sent\r\n",
           valid ? "PZEM data" : "PZEM error");
  } else {
    printf("USART1 -> Raspberry Pi: transmit failed (status %d)\r\n",
           (int)txStatus);
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_UART4_Init();
  /* USER CODE BEGIN 2 */
#if UART4_LOOPBACK_TEST
  printf("UART4 loopback test: disconnect PZEM and connect PA0 to PC11.\r\n");
  while (1) {
    uint8_t txByte = 0x55;
    uint8_t rxByte = 0;
    HAL_StatusTypeDef txStatus = HAL_UART_Transmit(&huart4, &txByte, 1, 100);
    HAL_StatusTypeDef rxStatus = HAL_UART_Receive(&huart4, &rxByte, 1, 100);

    if (txStatus == HAL_OK && rxStatus == HAL_OK && rxByte == txByte) {
      printf("USART4 LOOPBACK OK: 0x%02X\r\n", rxByte);
    } else {
      printf("USART4 LOOPBACK FAIL: TX=%d RX=%d DATA=0x%02X\r\n",
             (int)txStatus, (int)rxStatus, rxByte);
    }
    HAL_Delay(1000);
  }
#endif

  // PZEM on UART4 (PA0/PC11), terminal output on USART2 (PA2/PA3)
  printf("\r\n=== Smart Energy Monitor ===\r\n");
  printf("USART1: Raspberry Pi @ 115200\r\n");
  printf("USART2: Terminal @ 115200 (PA2/PA3)\r\n");
  printf("UART4: PZEM-004T @ 9600 (PA0/PC11)\r\n\r\n");

  const uint8_t pzemAddresses[] = {0x01, 0xF8};
  bool pzem_found = false;
  for (uint8_t addressIndex = 0;
       addressIndex < (sizeof(pzemAddresses) / sizeof(pzemAddresses[0])) && !pzem_found;
       addressIndex++) {
    PZEM_Init(&pzem, &huart4, pzemAddresses[addressIndex]);
    printf("Checking PZEM at address 0x%02X...\r\n", pzem.addr);

    for (uint8_t attempt = 1; attempt <= STARTUP_ATTEMPTS; attempt++) {
      if (PZEM_ReadAll(&pzem)) {
        printf("PZEM detected at 0x%02X on attempt %u! Voltage: %.1fV\r\n", // @suppress("Float formatting support")
               pzem.addr, (unsigned int)attempt, PZEM_Voltage(&pzem));
        pzem_found = true;
        break;
      }

      printf("Address 0x%02X: attempt %u/%u failed.\r\n",
             pzem.addr, (unsigned int)attempt, (unsigned int)STARTUP_ATTEMPTS);
      HAL_Delay(500);
    }
  }

  if (!pzem_found) {
    printf("WARNING: PZEM not responding; check wiring and address.\r\n");
  }

  printf("Polling every %u ms...\r\n\r\n", (unsigned int)POLL_INTERVAL);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  // Read all parameters from PZEM
	    	          bool read_ok = false;
	    	          for (uint8_t retry = 0; retry < MAX_RETRIES; retry++) {
	    	              if (PZEM_ReadAll(&pzem)) {
	    	                  read_ok = true;
	    	                  break;
	    	              }
	    	          }
	    	          if (read_ok) {
		 	              float voltage = PZEM_Voltage(&pzem);
		 	              float current = PZEM_Current(&pzem);
		 	              float power = PZEM_Power(&pzem);
		 	              float energy = PZEM_Energy(&pzem);
		 	              float frequency = PZEM_Frequency(&pzem);
		 	              float pf = PZEM_PowerFactor(&pzem);
                    SendPzemToRaspberry(true, voltage, current, power, energy, frequency, pf);

		 	              printf("V: %.1fV | I: %.3fA | P: %.1fW | E: %.3fkWh | F: %.1fHz | PF: %.2f\n", // @suppress("Float formatting support")
		 	                     voltage, current, power, energy, frequency, pf);
		 	          } else {
                    SendPzemToRaspberry(false, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		 	              printf("PZEM read failed!\n");
		 	          }

	    	          HAL_Delay(POLL_INTERVAL);

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 9600;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

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
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

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
