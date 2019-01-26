/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_hal.h"

/* USER CODE BEGIN Includes */
#include "dallas.h"
#include "flash_db.h"
#include "serial.h"
#include "onewire.h"
#include "ram_db.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
/* System millisecond counter */
uint32_t OS_Msec;

/* Echo flag */
uint32_t OS_Echo;

/* Flash log flag */
uint32_t OS_FlashLog;

/* Last conversion timestamp */
uint32_t OS_LastConversionTs;

/* Conversion period index */
uint32_t OS_PeriodIdx;

/* Conversion period look-up table in sek*/
static const uint32_t OS_PeriodLUT [  ] =
{
	1, 2, 5, 10, 15, 20, 30,
	45, 60, 90, 120, 180, 240,
	300, 450, 600, 900, 1200,
	1800, 2700, 3600, 5400,
	7200
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM1_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
static void PrintSamplingTime ( uint32_t sampling_time );
static uint8_t PrintTimeElement ( uint32_t value, const char* str );
static void UserInput ( char chr );
static uint8_t PrintTempColumns ( uint16_t T, uint8_t col );
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

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
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

	FDB_Init (  );
	RDB_Init (  );
	S_Init (  );
	DS_Init (  );

	OS_LastConversionTs = OS_Msec;
	printf ( "=======================\r\n");
	printf ( "========TERMOMETR======\r\n");
	printf ( "=======================\r\n");
	printf ( "Komendy: \r\n [-] Zmniejsz czas probkowania \r\n [+] Zwieksz czas probkowania \r\n" );
	printf ( " [e] Pokaz aktualna temperature \r\n [l] Zapis do pamieci FLASH \r\n" );
	printf ( " [f] Pokaz dane z pamieci FLASH \r\n [r] Pokaz dane z pamieci RAM\r\n" );
	printf ( " [c] Wyczysc pamiec FLASH \r\n" );
	printf ( " [?] Pokaz informacje \r\n" );
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

		if ( S_IsNewData (  ) )
		{
			UserInput ( S_GetCommand (  ) );
		}

		if ( GetTimeDiff ( OS_LastConversionTs ) >= OS_PeriodLUT [ OS_PeriodIdx ] * TICKS_PER_SEC )
		{
			uint16_t T = DS_LaunchConversion (  );
			OS_LastConversionTs = OS_Msec;

			RDB_RegisterVal ( T );

			if ( OS_FlashLog )
			{
				FDB_RegisterVal ( T );
			}

			if ( OS_Echo )
			{
				DS_PrintTemp ( T );
				printf ( "\r\n" );
			}
		}

		DS_DoWork (  );
	}
  /* USER CODE END 3 */

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Initializes the CPU, AHB and APB busses clocks
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* TIM1 init function */
static void MX_TIM1_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 31;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 0xFFFF;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART2 init function */
static void MX_USART2_UART_Init(void)
{

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
    _Error_Handler(__FILE__, __LINE__);
  }

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

}

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
        * Free pins are configured automatically as Analog (this feature is enabled through
        * the Code Generation settings)
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ONEWIRE_GPIO_Port, ONEWIRE_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, A_Pin|B_Pin|C_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PC13 PC14 PC15 PC0
                           PC1 PC2 PC3 PC4
                           PC5 PC6 PC7 PC8
                           PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_0
                          |GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8
                          |GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PD0 PD1 PD2 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA4 PA6
                           PA7 PA8 PA9 PA10
                           PA11 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_6
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LED1_Pin */
  GPIO_InitStruct.Pin = LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB10
                           PB11 PB12 PB14 PB15
                           PB3 PB4 PB5 PB6
                           PB7 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : ONEWIRE_Pin */
  GPIO_InitStruct.Pin = ONEWIRE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(ONEWIRE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : A_Pin B_Pin C_Pin */
  GPIO_InitStruct.Pin = A_Pin|B_Pin|C_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure peripheral I/O remapping */
  __HAL_AFIO_REMAP_PD01_ENABLE();

}

/* USER CODE BEGIN 4 */

static uint8_t PrintTimeElement ( uint32_t value, const char* str )
{
	if ( value > 0 )
	{
		printf ( "%u %s", ( unsigned int ) value, str );
		if ( value != 1 )
		{
			printf ( "s" );
		}
		return 1;
	}
	return 0;
}

static void PrintSamplingTime ( uint32_t sampling_time )
{
	static const uint32_t timeRadix = 60;
	uint32_t hours;
	uint32_t minutes;
	uint32_t seconds;

	hours = sampling_time / ( timeRadix * timeRadix );
	sampling_time -= hours * timeRadix * timeRadix;

	minutes = sampling_time / timeRadix;
	sampling_time -= minutes * timeRadix;

	seconds = sampling_time;

	if ( PrintTimeElement ( hours, "hour" ) && ( 0 != minutes || 0 != seconds ) )
	{
		printf ( " " );
	}

	if ( PrintTimeElement ( minutes, "min" ) && 0 != seconds )
	{
		printf ( " " );
	}

	PrintTimeElement ( seconds, "sek" );
}

static uint8_t PrintTempColumns ( uint16_t T, uint8_t col )
{
	uint8_t l = DS_PrintTemp ( T );

						/* Align */
						if ( l / 4 < 2 )
						{
							printf ( "\t\t" );
						}
						else
						{
							printf ( "\t" );
						}

						col++;
						if ( col == 4 )
						{
							col = 0;
							printf ( "\r\n" );
						}
						return ( col );
}

static void UserInput ( char chr )
{
	static const uint8_t periodLUTSize = sizeof ( OS_PeriodLUT ) / sizeof ( OS_PeriodLUT [ 0 ] );
	switch ( chr )
	{
		case '+':
		{
			if ( periodLUTSize - 1 <= OS_PeriodIdx )
			{
				OS_PeriodIdx = 0;
			}
			else
			{
				OS_PeriodIdx++;
			}
			printf ( "Interwal pomiarowy " );
			PrintSamplingTime ( OS_PeriodLUT [ OS_PeriodIdx ] );
			printf ( "\r\n" );

		}
		break;

		case '-':
		{
			if ( 0 == OS_PeriodIdx )
			{
				OS_PeriodIdx = periodLUTSize - 1;
			}
			else
			{
				OS_PeriodIdx--;
			}
			printf ( "Interwal pomiarowy " );
			PrintSamplingTime ( OS_PeriodLUT [ OS_PeriodIdx ] );
			printf ( "\r\n" );
		}
		break;

		case 'e':
		case 'E':
		{
			printf ("Sprawdzanie temperatury " );
			/* Toggle Echo */
			if ( OS_Echo )
			{
				OS_Echo = 0;
				printf ( "wylaczone" );
			}
			else
			{
				OS_Echo = 1;
				printf ( "wlaczone" );
			}
			printf ( "\r\n" );
		}
		break;

		case 'l':
		case 'L':
		{
			printf ("Zapisywanie do pamieci FLASH " );
			/* Toggle Flash Log */
			if ( OS_FlashLog )
			{
				OS_FlashLog = 0;
				printf ( "wylaczone" );
			}
			else
			{
				OS_FlashLog = 1;
				printf ( "wlaczone" );
			}
			printf ( "\r\n" );
		}
		break;

		case 'f':
		case 'F':
		{
			uint32_t col_cnt = 0;
			uint16_t T;
			printf ( "=======================\r\n");
			printf ( "=====Pamiec FLASH =====\r\n");
			printf ( "=======================\r\n");

			FDB_ResetReadTail (  );
			do
			{
				T = FDB_ReadData (  );
				if ( T != 0xFFFF )
				{
					col_cnt = PrintTempColumns ( T, col_cnt );
				}
			}	while ( T != 0xFFFF );
			printf ( "\r\n" );
		}
		break;

		case 'r':
		case 'R':
		{
			uint32_t col_cnt = 0;
						uint16_t T;
						printf ( "=======================\r\n");
						printf ( "======Pamiec RAM ======\r\n");
						printf ( "=======================\r\n");

						RDB_ResetReadTail (  );
						do
						{
							T = RDB_ReadData (  );
							if ( T != 0xFFFF )
							{
								col_cnt = PrintTempColumns ( T, col_cnt );
							}
						}	while ( T != 0xFFFF );
						printf ( "\r\n" );
		}
		break;

		case '?':
		{
			printf ( "=======================\r\n");
			printf ( "==========INFO=========\r\n" );
			printf ( "=======================\r\n");
			printf ( "Interwal pomiarowy " );
			PrintSamplingTime ( OS_PeriodLUT [ OS_PeriodIdx ] );
			printf ( "\r\n" );
			DS_PrintROM (  );
			printf ( "%u danych w pamieci FLASH \r\n", FDB_GetCountOfSamples (  ) );
			printf ( "%u danych w pamieci RAM \r\n", RDB_GetCountOfSamples (  ) );
		}
		break;

		case 'c':
		case 'C':
		{
			FDB_ClearMemory (  );
			printf ( "Wyczyszczono pamiec FLASH \r\n" );
		}
		break;

	}

}

uint32_t GetTimeDiff ( uint32_t timestamp )
{
	uint32_t diff;
	uint32_t now = OS_Msec;

	if ( now >= timestamp )
	{
		diff = now - timestamp;
	}
	else
	{
		diff = ( 0xFFFFFFFF - timestamp ) + now + 1;
	}

	return ( diff );
}

void HAL_SYSTICK_Callback ( void )
{
	OS_Msec++;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if ( htim == &htim1 )
	{
		OW_Callback_TimerEnd (  );
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	while(1)
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
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
