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
#include "fatfs.h"
#include "mdma.h"
#include "quadspi.h"
#include "sdmmc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"

#include <stdio.h>


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
extern  uint8_t QSPI_WriteEnable(void);
extern uint8_t QSPI_ResetChip(void);

typedef struct {
	char ssid[32];
	char password[32];
	char firmwareVersion[32];
	char serverIP[32];
	char defaultPage[32];
	uint16_t serverPort;
	uint16_t debounceHigh;
	uint16_t debounceLow;
} UserConfig_t;




__attribute__((section(".user_data"))) const UserConfig_t userConfig =
{
		.ssid = "teslateknoloji",
		.password = "tteessllaa",
		.firmwareVersion = "1.0.0",
		.serverIP = "192.168.1.145",
		.defaultPage = "tesla.aspx?",
		.serverPort = 51276,
		.debounceHigh = 100,
		.debounceLow = 50,
};

uint8_t buffer[512];
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

typedef void (*pFunction)(void);
pFunction JumpToApplication;
#define APPLICATION_ADDRESS  0x90000000

#define FW_FILENAME        "newfirmware.bin"
#define FW_BUFFER_SIZE     4096

/**QUADSPI GPIO Configuration
   PB6     ------> QUADSPI_BK1_NCS
   PF7     ------> QUADSPI_BK1_IO2
   PF6     ------> QUADSPI_BK1_IO3
   PF9     ------> QUADSPI_BK1_IO1
   PF8     ------> QUADSPI_BK1_IO0
   PB2     ------> QUADSPI_CLK
   */


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */




/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t writebuf[] = "Hello from memory";
uint8_t Readbuf[100];

int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart4, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

FRESULT UpdateFirmwareFromSD(void);
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	uint32_t i=0;

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
  MX_MDMA_Init();
  MX_QUADSPI_Init();
  MX_TIM4_Init();
  //MX_SDMMC1_SD_Init();
  //MX_FATFS_Init();
  //HAL_UART_MspDeInit(&huart4);
  //MX_UART4_Init();
  //HAL_UART_Transmit(&huart4,(uint8_t *)"SD mount failed\r\n", 17,HAL_MAX_DELAY);
  /* USER CODE BEGIN 2 */
 // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1|DISPLAY_ON_OFF_Pin, GPIO_PIN_SET);
 // HAL_GPIO_WritePin(GPIOB, BL_PWM_Pin|DISP_ON_Pin, GPIO_PIN_SET);

  HAL_GPIO_WritePin(DPoN_GPIO_Port, DPoN_Pin, GPIO_PIN_SET);//Dispaly OnOff Pini
  //Display BackLight PWM ve Data Enable Pinleri flashcode 2 de tanımlı
  // Bu OnOff Pinini On yapmazsak ekran siyah oluyor





  /* ===================================================================
   * SD card + firmware update — ŞİMDİLİK DEVRE DIŞI
   * --------------------------------------------------------------------
   * Boot süresini 25 saniyeye uzatıyor (HAL_SD_Init / f_mount yavaş).
   * Firmware download flow geliştirildiğinde, app SD'ye dosyayı yazınca
   * bir backup register flag set edip MCU'yu reset eder; bootloader
   * burada o flag'i okur, sadece set ise SD init + flash sequence çalışır,
   * normalde direkt QSPI memory-mapped'a alıp app'e jump eder.
   *
   * Tasarım taslağı (henüz uygulanmadı):
   *   if (HAL_PWR_CSDxxx & FW_UPDATE_FLAG) {
   *       // SD init + UpdateFirmwareFromSD()
   *       HAL_PWR_CSDxxx &= ~FW_UPDATE_FLAG;
   *   }
   * =================================================================== */

  /* bool sd_ok = false;
   * MX_SDMMC1_SD_Init();
   * MX_FATFS_Init();
   * uint32_t start = HAL_GetTick();
   * FATFS fs;
   * if (HAL_SD_Init(&hsd1) == HAL_OK) {
   *     if ((HAL_GetTick() - start) < 500) {
   *         if (f_mount(&fs, "", 1) == FR_OK) { sd_ok = true; }
   *     }
   * }
   * if (sd_ok) {
   *     HAL_QSPI_MspDeInit(&hqspi);
   *     HAL_QSPI_MspInit(&hqspi);
   *     if (CSP_QUADSPI_Init() == HAL_OK) {
   *         if (UpdateFirmwareFromSD() == FR_OK) {
   *             f_unlink(FW_FILENAME);
   *         }
   *     }
   *     f_mount(0, "", 0);
   * }
   * HAL_SD_Abort(&hsd1);
   * HAL_SD_DeInit(&hsd1);
   * HAL_SD_MspDeInit(&hsd1);
   * __HAL_RCC_SDMMC1_CLK_DISABLE();
   */

 //  ================= QSPI BOOT =================

  /*if (CSP_QUADSPI_Init() != HAL_OK)
      Error_Handler();

  CSP_QSPI_EnableMemoryMappedMode();

*/


/*

  FATFS fs;
  FIL file;
  FRESULT res;
  char line[128];
  char msg[160];

  HAL_UART_Transmit(&huart4,
                    (uint8_t *)"\r\n--- SD READ START ---\r\n",
                    26,
                    HAL_MAX_DELAY);

  // Mount SD
  res = f_mount(&fs, "", 1);
  if (res != FR_OK)
  {
      HAL_UART_Transmit(&huart4,
                        (uint8_t *)"SD mount failed\r\n",
                        17,
                        HAL_MAX_DELAY);
  }
  else
  {
      HAL_UART_Transmit(&huart4,
                        (uint8_t *)"SD mounted OK\r\n",
                        15,
                        HAL_MAX_DELAY);

      // Open file
      res = f_open(&file, "params.txt", FA_READ);
      if (res != FR_OK)
      {
          HAL_UART_Transmit(&huart4,
                            (uint8_t *)"Cannot open params.txt\r\n",
                            25,
                            HAL_MAX_DELAY);
      }
      else
      {
          while (f_gets(line, sizeof(line), &file))
          {
              line[strcspn(line, "\r\n")] = 0;

              snprintf(msg, sizeof(msg), "SD LINE: %s\r\n", line);
              HAL_UART_Transmit(&huart4,
                                (uint8_t *)msg,
                                strlen(msg),
                                HAL_MAX_DELAY);
          }

          f_close(&file);
      }

      f_mount(NULL, "", 1);
  }

  HAL_UART_Transmit(&huart4,
                    (uint8_t *)"--- SD READ END ---\r\n",
                    24,
                    HAL_MAX_DELAY);
while (1){}
*/



  /**QUADSPI GPIO Configuration
     PB6     ------> QUADSPI_BK1_NCS
     PF7     ------> QUADSPI_BK1_IO2
     PF6     ------> QUADSPI_BK1_IO3
     PF9     ------> QUADSPI_BK1_IO1
     PF8     ------> QUADSPI_BK1_IO0
     PB2     ------> QUADSPI_CLK
     */

  //HAL_QSPI_Abort(&hqspi);
  HAL_QSPI_MspDeInit(&hqspi);
  HAL_QSPI_MspInit(&hqspi);


 if (CSP_QUADSPI_Init() != HAL_OK) Error_Handler();

  /*  if (CSP_QSPI_Erase_Chip() != HAL_OK) Error_Handler();

    if (QSPI_WriteEnable() != HAL_OK) {
    			return HAL_ERROR;
    		}

   if ( BSP_QSPI_Write(writebuf,0,sizeof(writebuf)) != HAL_OK) Error_Handler();


  if( CSP_QSPI_Read(Readbuf,0,sizeof(writebuf)) != HAL_OK) Error_Handler();*/




 if (CSP_QSPI_EnableMemoryMappedMode() != HAL_OK) Error_Handler();





 //HAL_SD_DeInit(&hsd1);
 //__HAL_RCC_SDMMC1_CLK_DISABLE();

// HAL_QSPI_MspDeInit(&hqspi);
// HAL_QSPI_MspInit(&hqspi);


//if (CSP_QUADSPI_Init() != HAL_OK) Error_Handler();


// CSP_QSPI_EnableMemoryMappedMode();





 //SCB_DisableDCache();
 //SCB_DisableICache();
 SysTick->CTRL=0;
 __disable_irq();
 //HAL_QSPI_MspDeInit(&hqspi);

// HAL_RCC_DeInit();

   /* Clear Interrupt Enable Register & Interrupt Pending Register */

      for (i=0;i<5;i++)
      {
 	  NVIC->ICER[i]=0xFFFFFFFF;
 	  NVIC->ICPR[i]=0xFFFFFFFF;
      }

   /* Re-enable all interrupts */
     __enable_irq();
 JumpToApplication = (pFunction)(*(__IO uint32_t*)(APPLICATION_ADDRESS+4));
 __set_MSP(*(__IO uint32_t*)APPLICATION_ADDRESS);
 JumpToApplication();

/*
  memcpy(Readbuf, (uint8_t *) 0x90000000, sizeof(writebuf));

  if (CSP_QUADSPI_Init() != HAL_OK) Error_Handler();//exit memory mapped mode icin kullanıyoruz
  if ( BSP_QSPI_Write(writebuf,20,sizeof(writebuf)) != HAL_OK) Error_Handler();
  if( CSP_QSPI_Read(Readbuf,20,sizeof(writebuf)) != HAL_OK) Error_Handler();
  if (CSP_QSPI_EnableMemoryMappedMode() != HAL_OK) Error_Handler();
  memcpy(Readbuf, (uint8_t *) 0x90000000, 50);
*/

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	 // HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_1);
	  //yaklasik 1 saniye delay
	  //for(int i=0;i<5900;i++)
	  //{
	  	//for(int j=0;j<1000;j++){}

	  //}
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 128;
  RCC_OscInitStruct.PLL.PLLP = 10;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

FRESULT UpdateFirmwareFromSD(void)
{
	    FIL file;
	    UINT bytesRead;
	    uint32_t qspiAddr = 0;
	    static uint8_t buffer[FW_BUFFER_SIZE];

	    if (f_open(&file, FW_FILENAME, FA_READ) != FR_OK)
	        return FR_NO_FILE;

	    HAL_QSPI_Abort(&hqspi);

	    if (CSP_QSPI_Erase_Chip() != HAL_OK)
	        goto error;

	    do
	    {
	        if (f_read(&file, buffer, FW_BUFFER_SIZE, &bytesRead) != FR_OK)
	            goto error;

	        if (bytesRead > 0)
	        {
	            if (BSP_QSPI_Write(buffer, qspiAddr, bytesRead) != HAL_OK)
	                goto error;

	            qspiAddr += bytesRead;
	        }

	    } while (bytesRead == FW_BUFFER_SIZE);

	    f_close(&file);
	    return FR_OK;

	error:
	    f_close(&file);
	    return FR_INT_ERR;
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
#ifdef USE_FULL_ASSERT
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
