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
#include "usart.h"
#include "gpio.h"
#include "string.h"
#include "stdbool.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_RXBUF_SIZE 256
#define MAX_TXBUF_SIZE 256

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t usart1_rx_buf[MAX_RXBUF_SIZE];
uint8_t usart1_tx_buf[MAX_TXBUF_SIZE];
uint8_t usart2_rx_buf[MAX_RXBUF_SIZE];
uint8_t usart2_tx_buf[MAX_TXBUF_SIZE];

volatile uint16_t usart1_rx_len = 0;
volatile uint16_t usart2_rx_len = 0;
volatile bool usart1_rx_done = false;
volatile bool usart2_rx_done = false;

/* 初始化标志 */
volatile bool valve_initialized = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void RS485_SetTxMode(void);
void RS485_SetRxMode(void);
void Valve_SendCommand(uint8_t* data, uint16_t len);
void Valve_InitAddress(void);
void Valve_Task(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  RS485设置为发送模式 (DE/RE = 1)
  */
void RS485_SetTxMode(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
}

/**
  * @brief  RS485设置为接收模式 (DE/RE = 0)
  */
void RS485_SetRxMode(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
}

/**
  * @brief  通过USART2发送命令到阀门
  */
void Valve_SendCommand(uint8_t* data, uint16_t len)
{
    RS485_SetTxMode();
    HAL_Delay(1);
    HAL_UART_Transmit(&huart2, data, len, 0xFFFF);
    HAL_Delay(1);
    RS485_SetRxMode();
}

/**
  * @brief  初始化阀门地址（从00改为01）
  */
void Valve_InitAddress(void)
{
    // 设置阀门地址为01的命令
    // FF 20 00 20 00 01 02 00 01 2B 80
    uint8_t set_addr_cmd[] = {0xFF, 0x20, 0x00, 0x20, 0x00, 0x01, 0x02, 0x00, 0x01, 0x2B, 0x80};
    
    Valve_SendCommand(set_addr_cmd, sizeof(set_addr_cmd));
    
    HAL_Delay(100);
}

/**
  * @brief  阀门通信任务处理
  */
void Valve_Task(void)
{
    uint16_t rx_len;
    
    // 处理USART1接收到的数据
    if (usart1_rx_done)
    {
        usart1_rx_done = false;
        rx_len = usart1_rx_len;
        usart1_rx_len = 0;
        
        // 原样返回给PC
        HAL_UART_Transmit(&huart1, usart1_rx_buf, rx_len, 0xFFFF);
        
        // 如果收到数据是Modbus命令帧（包含地址和功能码），转发给阀门
        if (rx_len >= 4)
        {
            uint8_t addr = usart1_rx_buf[0];
            // 检查是否是有效的从站地址 (0x01-0xFE) 或广播地址 (0xFF)
            if ((addr >= 0x01 && addr <= 0xFE) || addr == 0xFF)
            {
                // 转发命令给阀门
                Valve_SendCommand(usart1_rx_buf, rx_len);
            }
        }
    }
    
    // 处理USART2接收到的阀门响应
    if (usart2_rx_done)
    {
        usart2_rx_done = false;
        rx_len = usart2_rx_len;
        usart2_rx_len = 0;
        
        // 将阀门响应原样返回给PC
        if (rx_len > 0)
        {
            HAL_UART_Transmit(&huart1, usart2_rx_buf, rx_len, 0xFFFF);
        }
    }
}

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
  
  // 初始化RS485为接收模式
  RS485_SetRxMode();
  
  // 启动USART1接收中断
  HAL_UART_Receive_IT(&huart1, &usart1_rx_buf[0], 1);
  
  // 启动USART2接收中断
  HAL_UART_Receive_IT(&huart2, &usart2_rx_buf[0], 1);
  
  // 初始化阀门地址（从00设置为01）
  Valve_InitAddress();
  valve_initialized = true;
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // 处理阀门通信任务
    Valve_Task();
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

/* USART接收完成回调 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        usart1_rx_buf[usart1_rx_len++] = usart1_rx_buf[0];
        
        if (usart1_rx_len >= MAX_RXBUF_SIZE)
        {
            usart1_rx_done = true;
        }
        else
        {
            // 继续接收下一个字节
            HAL_UART_Receive_IT(&huart1, &usart1_rx_buf[usart1_rx_len], 1);
        }
    }
    else if (huart->Instance == USART2)
    {
        usart2_rx_buf[usart2_rx_len++] = usart2_rx_buf[0];
        
        if (usart2_rx_len >= MAX_RXBUF_SIZE)
        {
            usart2_rx_done = true;
        }
        else
        {
            // 继续接收下一个字节
            HAL_UART_Receive_IT(&huart2, &usart2_rx_buf[usart2_rx_len], 1);
        }
    }
}

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
