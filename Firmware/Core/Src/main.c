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
#include "fatfs.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "ssd1306_tests.h"
#include "ssd1306_fonts.h"
#include "display.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
  void (*draw)(void);
  uint8_t input;
  uint8_t parent_id;  
} display;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* Top Level / Home States */
#define HOME_PAGE_STATE           0
#define HOME_MUSIC_STATE     1
#define HOME_TIMER_STATE     2
#define HOME_ALARM_STATE     3

/* Music Sub-States */
#define MUSIC_HOME_STATE 4
#define MUSIC_PLAYLIST_STATE 5
#define MUSIC_ALBUM_STATE    6
#define MUSIC_ALLSONGS_STATE 7
#define MUSIC_ARTIST_STATE 14

/* Timer Sub-States */
#define TIMER_HOME_STATE 8
#define TIMER_PRESET_STATE   9
#define SET_TIMER_STATE      10

/* Alarm Sub-States */
#define ALARM_HOME_STATE 11
#define ALARM_PRESET_STATE   12
#define ALARM_SET_STATE      13


#define PLAY_PRESSED (HAL_GPIO_ReadPin(BUTTON_PLAY_GPIO_Port, BUTTON_PLAY_Pin) == GPIO_PIN_SET)
#define REWIND_PRESSED (HAL_GPIO_ReadPin(BUTTON_REWIND_GPIO_Port, BUTTON_REWIND_Pin) == GPIO_PIN_SET)
#define SKIP_PRESSED (HAL_GPIO_ReadPin(BUTTON_SKIP_GPIO_Port, BUTTON_SKIP_Pin) == GPIO_PIN_SET)
#define VOL_DOWN_PRESSED (HAL_GPIO_ReadPin(BUTTON_VOL_DOWN_GPIO_Port, BUTTON_VOL_DOWN_Pin) == GPIO_PIN_SET)
#define VOL_UP_PRESSED   (HAL_GPIO_ReadPin(BUTTON_VOL_UP_GPIO_Port, BUTTON_VOL_UP_Pin) == GPIO_PIN_SET)
#define SCROLL_BUTTON_PRESSED (HAL_GPIO_ReadPin(BUTTON_SCROLL_GPIO_Port, BUTTON_SCROLL_Pin) == GPIO_PIN_RESET)

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef hlpuart1;

SAI_HandleTypeDef hsai_BlockA1;
DMA_HandleTypeDef hdma_sai1_a;

SD_HandleTypeDef hsd1;
DMA_HandleTypeDef hdma_sdmmc1_rx;
DMA_HandleTypeDef hdma_sdmmc1_tx;

TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */
/*
  _____  _____  _______      __       _______ ______   __      __     _____  _____          ____  _      ______  _____ 
 |  __ \|  __ \|_   _\ \    / /   /\ |__   __|  ____|  \ \    / /\   |  __ \|_   _|   /\   |  _ \| |    |  ____|/ ____|
 | |__) | |__) | | |  \ \  / /   /  \   | |  | |__      \ \  / /  \  | |__) | | |    /  \  | |_) | |    | |__  | (___   
 |  ___/|  _  /  | |   \ \/ /   / /\ \  | |  |  __|      \ \/ / /\ \ |  _  /  | |   / /\ \ |  _ <| |    |  __|  \___ \  
 | |    | | \ \ _| |_   \  /   / ____ \ | |  | |____      \  / ____ \| | \ \ _| |_ / ____ \| |_) | |____| |____ ____) | 
 |_|    |_|  \_\_____|   \/   /_/    \_\|_|  |______|      \/_/    \_\_|  \_\_____/_/    \_\____/|______|______|_____/ 
                                                     
                                                                  
*/

//Initialisation Code
char test_song[20]="music.wav";
uint8_t current_state = 0;
uint8_t next_state = 0;
bool down_button_last = false;


// SDMMC
FATFS MyFatFS; 
FIL MyFile;    
FRESULT res;   
uint32_t byteswritten;

// Audio
#define AUDIO_BUF_SIZE 4096 
#define READBUF_SIZE 4096
uint16_t audio_buffer[AUDIO_BUF_SIZE]; // The main buffer
uint8_t playing = 0;                   // State flag
UINT bytes_read;                       // To track SD card progress
uint8_t readBuf[READBUF_SIZE]; // Buffer for encoded MP3 data
int16_t outBuf[2 * 1152];      // Buffer for decoded PCM data
uint32_t adc_buffer[1];
uint8_t timer_ready = 0;
uint8_t last_button_state = 0; 

// Display
uint8_t music_option = 0;


// Miscellaneous (Buttons, Sensors, States etc.)
uint8_t music_volume = 0;
uint8_t previous_volume = 0;
int8_t scroll_value = 0;
int8_t previous_scroll_value = 0;
uint8_t refresh_needed = 0;
uint8_t scroll_button_hold = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_I2C1_Init(void);
static void MX_SDMMC1_SD_Init(void);
static void MX_SAI1_Init(void);
static void MX_ADC1_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/*
  _____  _____  _______      __       _______ ______   _    _  _____ ______ _____     _____  ____  _____  ______  
 |  __ \|  __ \|_   _\ \    / /   /\ |__   __|  ____| | |  | |/ ____|  ____|  __ \   / ____|/ __ \|  __ \|  ____| 
 | |__) | |__) | | |  \ \  / /   /  \   | |  | |__    | |  | | (___ | |__  | |__) | | |    | |  | | |  | | |__   
 |  ___/|  _  /  | |   \ \/ /   / /\ \  | |  |  __|   | |  | |\___ \|  __| |  _  /  | |    | |  | | |  | |  __|  
 | |    | | \ \ _| |_   \  /   / ____ \ | |  | |____  | |__| |____) | |____| | \ \  | |____| |__| | |__| | |____ 
 |_|    |_|  \_\_____|   \/   /_/    \_\|_|  |______|  \____/|_____/|______|_|  \_\  \_____|\____/|_____/|______|
                                                                                   
                              
*/

// void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
//   timer_ready = 1;
// 	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);

// }

void TestInputsOnly() {
  if (PLAY_PRESSED || REWIND_PRESSED || SKIP_PRESSED || VOL_DOWN_PRESSED || VOL_UP_PRESSED || SCROLL_BUTTON_PRESSED) HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
  else HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
}

void TestInputsAndDisplay() {
  uint8_t x = 2;
  ssd1306_Fill(Black);
  #ifdef SSD1306_INCLUDE_FONT_7x10
  ssd1306_SetCursor(2, 25);

  if (PLAY_PRESSED) ssd1306_WriteString("Play", Font_7x10, White);
  else if (REWIND_PRESSED) ssd1306_WriteString("Rewind", Font_7x10, White);
  else if (SKIP_PRESSED) ssd1306_WriteString("Skip", Font_7x10, White);
  else if (VOL_DOWN_PRESSED) ssd1306_WriteString("Volume Down", Font_7x10, White);
  else if (VOL_UP_PRESSED) ssd1306_WriteString("Volume Up", Font_7x10, White);
  else if (SCROLL_BUTTON_PRESSED) ssd1306_WriteString("Scroll Pressed", Font_7x10, White);
  
  #endif
  ssd1306_UpdateScreen();
}

void PlaySong(char song[20]) {
    res = f_mount(&MyFatFS, (TCHAR const *)SDPath, 1);
  if (res == FR_OK) {
    // 2. Open the audio file
    res = f_open(&MyFile, "music.wav", FA_READ);
    if (res == FR_OK) {
      // 3. Skip WAV header (44 bytes)
      f_lseek(&MyFile, 44);
      // 4. Initial Buffer Fill (Read bytes, but fill half the samples)
      // We read AUDIO_BUF_SIZE * 2 because each sample is 2 bytes (uint16_t)
      res = f_read(&MyFile, audio_buffer, AUDIO_BUF_SIZE * 2, &bytes_read);
      if (res == FR_OK && bytes_read > 0) {
        // 5. Start SAI DMA (SAI expects number of samples, not bytes)
        HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t *)audio_buffer,
                             AUDIO_BUF_SIZE);
        playing = 1;
      }
    } else {
      // Show error if file not found
      char errBuf[20];
      sprintf(errBuf, "Open Error: %d", res);
      ssd1306_SetCursor(2, 30);
      ssd1306_WriteString(errBuf, Font_7x10, White);
      ssd1306_UpdateScreen();
    }
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

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_LPUART1_UART_Init();
  MX_USB_HOST_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  MX_SDMMC1_SD_Init();
  MX_FATFS_Init();
  MX_SAI1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

/*
  _____ _   _ _____ _______ _____          _      _____  _____         _______  _____  ____  _   _  _____ 
 |_   _| \ | |_   _|__   __|_   _|   /\   | |    |_   _|/ ____|  / \  |__    __|_   _|/ __ \| \ | |/ ____|
   | | |  \| | | |    | |    | |    /  \  | |      | | | (___   /   \    |  |    | | | |  | |  \| | (___  
   | | | . ` | | |    | |    | |   / /\ \ | |      | |  \___ \ /  /\ \   |  |    | | | |  | | . ` |\___ \ 
  _| |_| |\  |_| |_   | |   _| |_ / ____ \| |____ _| |_ ____) /  ____ \  |  |   _| |_| |__| | |\  |____) |
 |_____|_| \_|_____|  |_|  |_____/_/    \_\______|_____|_____/_/     \ \_|__|  |_____|\____/|_| \_|_____/ 
                                                                                                       
*/
  
// Initialisations
  HAL_TIM_Base_Start_IT(&htim1);

  // Display Initialisation
    ssd1306_Init();
  //DrawHomePage(music_volume);
  //PlaySong(test_song);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  /*
 __          _   _    _ _____ _      ______   _      ____   ____  _____  
 \ \        / / | |  | |_   _| |    |  ____| | |    / __ \ / __ \|  __ \ 
  \ \  /\  / /  | |__| | | | | |    | |__    | |   | |  | | |  | | |__) |
   \ \/  \/ /   |  __  | | | | |    |  __|   | |   | |  | | |  | |  ___/ 
    \  /\  /    | |  | |_| |_| |____| |____  | |___| |__| | |__| | |     
     \/  \/     |_|  |_|_____|______|______| |______\____/ \____/|_|     
                                                                       
*/
  while (1) {
    //TestInputs();
    TestInputsAndDisplay();
// // 1. Calculate the change (Delta)
// int8_t delta = scroll_value - previous_scroll_value;
// switch (current_state) {
//   case HOME_PAGE_STATE:
//     if (delta !=0) {
//       if ( delta > 0) next_state = HOME_MUSIC_STATE;
//       else if (delta < 0) next_state = HOME_ALARM_STATE; 
//       refresh_needed = 1;
//     }
//     break;
//   case HOME_MUSIC_STATE:
//     if (delta !=0) {
//       if ( delta > 0) next_state = HOME_TIMER_STATE;
//       else if (delta < 0) next_state = HOME_PAGE_STATE;
//       refresh_needed = 1;
//     }
//     if (SCROLL_BUTTON_PRESSED) next_state = MUSIC_HOME_STATE;

//     break;
//   case HOME_TIMER_STATE:
//     if (delta !=0) {
//       if ( delta > 0) next_state = HOME_ALARM_STATE;
//       else if (delta < 0) next_state = HOME_MUSIC_STATE; 
//     }
//     if (SCROLL_BUTTON_PRESSED) next_state = TIMER_HOME_STATE;
//     refresh_needed = 1;
//     break;
//   case HOME_ALARM_STATE:
//     if (delta !=0) {
//       if ( delta > 0) next_state = HOME_PAGE_STATE;
//       else if (delta < 0) next_state = HOME_TIMER_STATE; 
//       refresh_needed = 1;
//     }
//     if (SCROLL_BUTTON_PRESSED) next_state = ALARM_HOME_STATE;

//     break;
  
// }

// if (current_state != next_state) {
//     current_state = next_state;
//     refresh_needed = 1; // Force redraw on state change
// }
// // Only Redraw if the state actually changed to save I2C bandwidth
// if (refresh_needed) {
//   refresh_needed = 0;
//     switch (current_state) {
//         case HOME_PAGE_STATE:  
//           DrawHomePage(music_volume); 
//           break;
//         case HOME_MUSIC_STATE: DrawMusicPlayerIcon(); break;
//         case MUSIC_HOME_STATE: DrawMusicPlayerHomePage(music_option); break;
//         case MUSIC_ALLSONGS_STATE: PlaySong(test_song); break;
//         case HOME_TIMER_STATE: 
//           DrawTimerIcon(); 
//           break;
//         case HOME_ALARM_STATE: 
//           DrawAlarmIcon(); 
//           break;
//     }
// }
// delta = 0;

// if (delta != 0) {
//     refresh_needed = 1;
//     // 2. Update the state based on direction
//     if (delta > 0) { // Clockwise
//       switch (current_state) {
//           case HOME_PAGE_STATE:  next_state = HOME_MUSIC_STATE; break;
//           case HOME_MUSIC_STATE: next_state = HOME_TIMER_STATE; break;
//           case HOME_TIMER_STATE: next_state = HOME_ALARM_STATE; break;
//           case HOME_ALARM_STATE: next_state = HOME_PAGE_STATE;  break;
//           case MUSIC_HOME_STATE: break;
//       }
//       if (music_option < 4) {
//         music_option++;
//       }
//     } 
//     else { // Counter-Clockwise
//         switch (current_state) {
//             case HOME_PAGE_STATE:  next_state = HOME_ALARM_STATE; break;
//             case HOME_MUSIC_STATE: next_state = HOME_PAGE_STATE;  break;
//             case HOME_TIMER_STATE: next_state = HOME_MUSIC_STATE; break;
//             case HOME_ALARM_STATE: next_state = HOME_TIMER_STATE; break;
//         }
//         if (music_option > 0) {
//           music_option--;
//         }
//     }
//     // 3. Sync the values so we don't trigger again until the next move
//     previous_scroll_value = scroll_value;
// }

// if (previous_volume != music_volume) {
//   DrawVolume(music_volume);
//   previous_volume = music_volume;
// }


// if (SCROLL_BUTTON_PRESSED) {
//   refresh_needed = 1;
//   switch (current_state) {
//     case HOME_MUSIC_STATE: next_state = MUSIC_HOME_STATE;  break;
//     case MUSIC_HOME_STATE:
//       switch (music_option) {
//         case ALL_SONGS_OPTION: next_state = MUSIC_ALLSONGS_STATE; break;
//         case PLAYLISTS_OPTION: next_state = MUSIC_PLAYLIST_STATE; break;
//         case ALBUMS_OPTION: next_state = MUSIC_ALBUM_STATE; break;
//         case ARTISTS_OPTION: next_state = MUSIC_ARTIST_STATE; break;
//       }
//       break;
//     case MUSIC_PLAYLIST_STATE: break;
//     case MUSIC_ALBUM_STATE: break;
//     case MUSIC_ARTIST_STATE: break;
//     case HOME_TIMER_STATE: next_state = TIMER_HOME_STATE; break;
//     case TIMER_HOME_STATE: break;
//     case HOME_ALARM_STATE: next_state = ALARM_HOME_STATE; break;
//     case ALARM_HOME_STATE: break;
//   }
// }


// if (current_state != next_state) {
//     current_state = next_state;
//     refresh_needed = 1; // Force redraw on state change
// }
// // Only Redraw if the state actually changed to save I2C bandwidth
// if (refresh_needed) {
//   refresh_needed = 0;
//     switch (current_state) {
//         case HOME_PAGE_STATE:  
//           DrawHomePage(music_volume); 
//           break;
//         case HOME_MUSIC_STATE: DrawMusicPlayerIcon(); break;
//         case MUSIC_HOME_STATE: DrawMusicPlayerHomePage(music_option); break;
//         case MUSIC_ALLSONGS_STATE: PlaySong(test_song); break;
//         case HOME_TIMER_STATE: 
//           DrawTimerIcon(); 
//           break;
//         case HOME_ALARM_STATE: 
//           DrawAlarmIcon(); 
//           break;
//     }
// }
    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_9;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 71;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV4;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SAI1|RCC_PERIPHCLK_USB
                              |RCC_PERIPHCLK_SDMMC1|RCC_PERIPHCLK_ADC;
  PeriphClkInit.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLLSAI1;
  PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLLSAI1;
  PeriphClkInit.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_PLLSAI1;
  PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_MSI;
  PeriphClkInit.PLLSAI1.PLLSAI1M = 5;
  PeriphClkInit.PLLSAI1.PLLSAI1N = 20;
  PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_SAI1CLK|RCC_PLLSAI1_48M2CLK
                              |RCC_PLLSAI1_ADC1CLK;
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

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00100208;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */
  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 209700;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_7B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief SAI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SAI1_Init(void)
{

  /* USER CODE BEGIN SAI1_Init 0 */

  /* USER CODE END SAI1_Init 0 */

  /* USER CODE BEGIN SAI1_Init 1 */

  /* USER CODE END SAI1_Init 1 */
  hsai_BlockA1.Instance = SAI1_Block_A;
  hsai_BlockA1.Init.AudioMode = SAI_MODEMASTER_TX;
  hsai_BlockA1.Init.Synchro = SAI_ASYNCHRONOUS;
  hsai_BlockA1.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  hsai_BlockA1.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  hsai_BlockA1.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
  hsai_BlockA1.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_44K;
  hsai_BlockA1.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
  hsai_BlockA1.Init.MonoStereoMode = SAI_STEREOMODE;
  hsai_BlockA1.Init.CompandingMode = SAI_NOCOMPANDING;
  hsai_BlockA1.Init.TriState = SAI_OUTPUT_NOTRELEASED;
  if (HAL_SAI_InitProtocol(&hsai_BlockA1, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_16BIT, 2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SAI1_Init 2 */

  /* USER CODE END SAI1_Init 2 */

}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_1B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv = 16;
  /* USER CODE BEGIN SDMMC1_Init 2 */

  /* USER CODE END SDMMC1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 14199;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA2_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel1_IRQn);
  /* DMA2_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel4_IRQn);
  /* DMA2_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel5_IRQn);

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, USB_PowerSwitchOn_Pin|SMPS_V1_Pin|SMPS_EN_Pin|SMPS_SW_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BUTTON_REWIND_Pin */
  GPIO_InitStruct.Pin = BUTTON_REWIND_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(BUTTON_REWIND_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BUTTON_VOL_UP_Pin BUTTON_VOL_DOWN_Pin */
  GPIO_InitStruct.Pin = BUTTON_VOL_UP_Pin|BUTTON_VOL_DOWN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : BUTTON_SCROLL_Pin */
  GPIO_InitStruct.Pin = BUTTON_SCROLL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BUTTON_SCROLL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SCROLL_S1_Pin */
  GPIO_InitStruct.Pin = SCROLL_S1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SCROLL_S1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SCROLL_S2_Pin */
  GPIO_InitStruct.Pin = SCROLL_S2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SCROLL_S2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BUTTON_SKIP_Pin BUTTON_PLAY_Pin */
  GPIO_InitStruct.Pin = BUTTON_SKIP_Pin|BUTTON_PLAY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_OverCurrent_Pin SMPS_PG_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin|SMPS_PG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_PowerSwitchOn_Pin SMPS_V1_Pin SMPS_EN_Pin SMPS_SW_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin|SMPS_V1_Pin|SMPS_EN_Pin|SMPS_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* This fills the FIRST half (Index 0 to BUF_SIZE/2 - 1) */
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai) {
    // We read (Half the samples) * (2 bytes per sample)
    f_read(&MyFile, &audio_buffer[0], AUDIO_BUF_SIZE, &bytes_read);
}

/* This fills the SECOND half (Index BUF_SIZE/2 to BUF_SIZE - 1) */
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai) {
    // We read starting at the middle index
    f_read(&MyFile, &audio_buffer[AUDIO_BUF_SIZE/2], AUDIO_BUF_SIZE, &bytes_read);
    
    // Check if the amount read is less than a half-buffer
    if (bytes_read < AUDIO_BUF_SIZE) {
        HAL_SAI_DMAStop(&hsai_BlockA1);
        f_close(&MyFile);
        playing = 0;
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /* 1. SCROLL WHEEL (PD8) */
    switch (GPIO_Pin) {
      case SCROLL_S1_Pin:
        // Direction check using S2 (PD9)
        (HAL_GPIO_ReadPin(SCROLL_S2_GPIO_Port, SCROLL_S2_Pin) == GPIO_PIN_SET) ? scroll_value-- : scroll_value++;
        break;
      case BUTTON_VOL_UP_Pin:
          if (music_volume < 20) {
              music_volume++;
          }
          break;

      case BUTTON_VOL_DOWN_Pin:
          if (music_volume > 0) {
              music_volume--;
          }
          break;
      case BUTTON_SCROLL_Pin:
        if (HAL_GPIO_ReadPin(BUTTON_SCROLL_GPIO_Port, BUTTON_SCROLL_Pin) == GPIO_PIN_SET) {
            // Button just pressed: Start/Reset Timer
            __HAL_TIM_SET_COUNTER(&htim1, 0); 
            HAL_TIM_Base_Start(&htim1);
        } else {
            // Button released: Read Timer
            uint32_t duration = __HAL_TIM_GET_COUNTER(&htim1);
            HAL_TIM_Base_Stop(&htim1);

            if (duration > 1500) {
                scroll_button_hold = 1;
            } else if (duration > 50) {
                scroll_button_hold = 0;
            }
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
