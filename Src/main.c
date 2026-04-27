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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
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

/* USER CODE BEGIN PV */
uint8_t rs485_rx_buf[100];
int16_t rs485_rx_size = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t buffer[100];
uint16_t size = 0;

/**
 * @brief 将ASCII十六进制字符串转换为字节数组
 * @param hex_str: 输入字符串，如 "01 06 00 01 13 88 D5 5C" 或 "010600011388D55C"
 * @param output: 输出字节数组
 * @return 转换后的字节数
 */
int HexStringToBytes(uint8_t *hex_str, uint8_t *output, int max_len)
{
    int len = strlen((char *)hex_str);
    int j = 0;
    
    for (int i = 0; i < len && j < max_len; i++)
    {
        // 跳过空格
        if (hex_str[i] == ' ' || hex_str[i] == '\r' || hex_str[i] == '\n')
            continue;
        
        // 确保还有下一个字符
        if (i + 1 >= len) break;
        
        uint8_t high, low;
        
        // 解析高4位
        if (hex_str[i] >= '0' && hex_str[i] <= '9')
            high = hex_str[i] - '0';
        else if (hex_str[i] >= 'A' && hex_str[i] <= 'F')
            high = hex_str[i] - 'A' + 10;
        else if (hex_str[i] >= 'a' && hex_str[i] <= 'f')
            high = hex_str[i] - 'a' + 10;
        else
            continue; // 非法字符，跳过
        
        // 解析低4位
        if (hex_str[i+1] >= '0' && hex_str[i+1] <= '9')
            low = hex_str[i+1] - '0';
        else if (hex_str[i+1] >= 'A' && hex_str[i+1] <= 'F')
            low = hex_str[i+1] - 'A' + 10;
        else if (hex_str[i+1] >= 'a' && hex_str[i+1] <= 'f')
            low = hex_str[i+1] - 'a' + 10;
        else
            continue; // 非法字符，跳过
        
        output[j++] = (high << 4) | low;
        i++; // 跳过下一个字符
    }
    
    return j;
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
  /* USER CODE BEGIN 2 */

  // 发送一个字符串
  HAL_UART_Transmit(&huart1, "RS485 MODBUS Init...\n", 20, 1000);

  // 修改阀门地址从00到01
  uint8_t set_addr_cmd[] = {0xFF, 0x20, 0x00, 0x20, 0x00, 0x01, 0x02, 0x00, 0x01, 0x2B, 0x80};
  HAL_UART_Transmit(&huart1, "Setting valve address 01...\n", 27, 1000);
  RS485_Send(set_addr_cmd, 11);
  HAL_Delay(100);
  rs485_rx_size = RS485_Receive(rs485_rx_buf, 100, 500);
  if (rs485_rx_size > 0)
  {
    HAL_UART_Transmit(&huart1, "Address set OK: ", 16, 100);
    HAL_UART_Transmit(&huart1, rs485_rx_buf, rs485_rx_size, 100);
    HAL_UART_Transmit(&huart1, (uint8_t *)"\n", 1, 100);
  }
  else
  {
    HAL_UART_Transmit(&huart1, "Address set failed, try anyway...\n", 31, 1000);
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // 接收一个变长字符串，然后通过 RS485 转发出去
    if (HAL_UARTEx_ReceiveToIdle(&huart1, buffer, 100, &size, 100) == HAL_OK)
    {
      uint8_t cmd_buf[100];
      
      // 将ASCII十六进制字符串转换为字节
      int cmd_len = HexStringToBytes(buffer, cmd_buf, 100);
      
      if (cmd_len > 0)
      {
        // 打印转换后的命令
        char tx_info[50];
        int tx_len = sprintf(tx_info, "\r\nTX(%d): ", cmd_len);
        HAL_UART_Transmit(&huart1, (uint8_t *)tx_info, tx_len, 100);
        
        // 十六进制显示发送数据
        char hex_str[300];
        int hex_len = 0;
        for (int i = 0; i < cmd_len; i++)
        {
          hex_len += sprintf(hex_str + hex_len, "%02X ", cmd_buf[i]);
        }
        HAL_UART_Transmit(&huart1, (uint8_t *)hex_str, hex_len, 100);

        // 通过 RS485 发送数据到电动阀
        RS485_Send(cmd_buf, cmd_len);

        // 等待电动阀响应
        HAL_Delay(50);
        rs485_rx_size = RS485_Receive(rs485_rx_buf, 100, 500);
        if (rs485_rx_size > 0)
        {
          // 打印电动阀响应
          char rx_info[50];
          int rx_len = sprintf(rx_info, "\r\nRX(%d): ", rs485_rx_size);
          HAL_UART_Transmit(&huart1, (uint8_t *)rx_info, rx_len, 100);
          
          // 十六进制显示响应数据
          hex_len = 0;
          for (int i = 0; i < rs485_rx_size; i++)
          {
            hex_len += sprintf(hex_str + hex_len, "%02X ", rs485_rx_buf[i]);
          }
          HAL_UART_Transmit(&huart1, (uint8_t *)hex_str, hex_len, 100);
        }
        else
        {
          HAL_UART_Transmit(&huart1, (uint8_t *)"\r\nRX: Timeout!", 14, 100);
        }
      }
      else
      {
        HAL_UART_Transmit(&huart1, (uint8_t *)"\r\nInvalid hex format!", 20, 100);
      }
    }

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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
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
