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
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <ctype.h>
#include "usbd_custom_hid_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef union {
	  uint8_t  buf[4];
	  struct {
		  uint32_t status:8;	// not SPI transfered
		  uint32_t number:20;
		  uint32_t sign:1;		// 1 = negative
		  uint32_t p2:2;
		  uint32_t unit:1;		// 1 = inches
	  } s;
}DATA;

typedef enum {
	IDLE,
	WAIT,
	SYNC,
	START,
	RECV,
	SEND,
	STOP
} state;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define EMPTY 0
#define RECVD 1
#define ERROR 2


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
IWDG_HandleTypeDef hiwdg;

SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */

char userbuf[USBD_CUSTOMHID_OUTREPORT_BUF_SIZE];
DATA data;

uint8_t spi_buf[3];

extern uint64_t _estack;

volatile state st = IDLE;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_IWDG_Init(void);
/* USER CODE BEGIN PFP */
int8_t USBD_CUSTOM_HID_SendReport_FS(uint8_t *report, uint16_t len);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void feed_watchdog()
{
	static uint32_t timeout = 1000;
	uint32_t ctime = HAL_GetTick();
	if (timeout < ctime)
	{
		HAL_IWDG_Refresh(&hiwdg);
		timeout = 1000 + ctime;
	}
}

static inline void clear_data()
{
	memset(&data, EMPTY, sizeof(data));
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi->Instance==SPI1)
	{
		if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_MODF) || __HAL_SPI_GET_FLAG(hspi, SPI_FLAG_OVR))
		{
			if(RECV == st)
				st = STOP;
		}
		else
		{
			if(RECV == st)
			{
				memcpy(data.buf + 1, spi_buf, sizeof(spi_buf));
				st = SEND;
			}
		}
		/* must be after any transmit to work (why?)*/
		__HAL_SPI_CLEAR_MODFFLAG(hspi);
		__HAL_SPI_CLEAR_OVRFLAG(hspi);
	}
}

void send(char *ch)
{
	uint8_t report[8] = {2,0,74,77};	// shift+home+end

	USBD_CUSTOM_HID_SendReport_FS(report, sizeof(report));
	feed_watchdog();
	HAL_Delay(32);
	feed_watchdog();
	memset(report, 0, sizeof(report));
	USBD_CUSTOM_HID_SendReport_FS(report, sizeof(report));
	feed_watchdog();
	HAL_Delay(32);
	feed_watchdog();

	while (*ch)
	{
		uint8_t i = 2;
		while (i < 8 && *ch)
		{
			if (isdigit((int)*ch))
			{
				report[i] = (*ch - 39) % 10 + 89;
			}
			else
			{
				switch (*ch)
				{
				case 'i': report[i] = 12; break;
				case 'm': report[i] = 16; break;
				case 'n': report[i] = 17; break;
				case ' ': report[i] = 44; break;
				case '-': report[i] = 86; break;
				case '+': report[i] = 87; break;
				case '.': report[i] = 99; break;
				default : ch++; continue;
				};
			}
			for (uint8_t j = 2; j <= i; j++)
			{
				if (i != j)
				{
					if (report[i] == report[j])
					{
						report[i] = 0;
						goto flush;
					}
				}
			}
			ch++;
			i++;
		}
		flush: USBD_CUSTOM_HID_SendReport_FS(report, sizeof(report));
		feed_watchdog();
		HAL_Delay(32);
		feed_watchdog();
		memset(report, 0, sizeof(report));
		USBD_CUSTOM_HID_SendReport_FS(report, sizeof(report));
		feed_watchdog();
		HAL_Delay(32);
	}
}

void data2string(const DATA *data, char *string, uint8_t len) {
	int val = data->s.number;
	if (data->s.unit) {
		val *= 5;
		snprintf(string, len, "%c%01i.%04i in",
				(data->s.sign ? '-' : '+'), val / 10000, val % 10000);
	} else {
		snprintf(string, len, "%c%03i.%02i mm",
				(data->s.sign ? '-' : '+'), val / 100, val % 100);
	}
}

int checkcmd(char *command)
{
	int retval = !strncmp(command,userbuf,sizeof(userbuf));
	if(retval)
		memset(userbuf,0,sizeof(userbuf));
	return retval;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

	char string[15];

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
  MX_SPI1_Init();
  MX_USB_DEVICE_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */

  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  uint32_t wait = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
		feed_watchdog();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

		switch(st)
		{
		case IDLE:
			if(checkcmd("load"))
			{
				_estack = 0xDEADBEEFCC00FFEEULL;
				HAL_NVIC_SystemReset();
			}
			if((GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5)) && checkcmd("send"))
			{
				clear_data();
				st = WAIT;
				wait = 1000 + HAL_GetTick();
			}
			break;
		case WAIT:
			if(wait < HAL_GetTick())
			{
				st = STOP;
			}
			if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5))
				st = SYNC;
			break;
		case SYNC:
			wait = 10 + HAL_GetTick();
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
			st = START;
			break;
		case START:
			if(wait < HAL_GetTick())
			{
				st = RECV;
				while(HAL_OK != HAL_SPI_Receive_IT(&hspi1, spi_buf, sizeof(spi_buf)));
				wait = 200 + HAL_GetTick();
			}
			break;
		case RECV:
			if(wait < HAL_GetTick())
			{
				st = STOP;
			}
			// waiting for data
			break;
		case SEND:
			data2string(&data, string, sizeof(string));
			send(string);
			st = STOP;
			break;
		case STOP:
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
			st = IDLE;
			break;
		}
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */
#if 1
  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */
#endif
  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_SLAVE;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES_RXONLY;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_LSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

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
