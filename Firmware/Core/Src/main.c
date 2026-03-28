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
#include "audio_control.h"
#include "input.h"
#include "ff.h"
// In stm32l4xx_it.c (or sai.c, wherever CubeMX put the SAI IRQ handler)
#include "music_player.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum {
  STATE_HOME,
  STATE_TIMER,
  STATE_TIMER_SET,
  STATE_TIMER_DONE,
  STATE_ALARM,
  STATE_MUSIC,
  STATE_SETTINGS,
} State_t;

typedef enum {
  NONE,
  EVENT_SCROLL_HOLD,
  EVENT_SCROLL_PRESS,
  EVENT_SCROLL_CLOCKWISE,
  EVENT_SCROLL_ANTICLOCKWISE,
} Event_t;

typedef struct {
  char *name;
  State_t next_screen;  
} menu_screen;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

//Debugging defines
#define LD3_ON  (HAL_GPIO_WritePin(LD3_GPIO_Port,LD3_Pin, GPIO_PIN_SET))
#define LD3_OFF  (HAL_GPIO_WritePin(LD3_GPIO_Port,LD3_Pin, GPIO_PIN_RESET))
#define LD2_ON  (HAL_GPIO_WritePin(LD2_GPIO_Port,LD2_Pin, GPIO_PIN_SET))
#define LD2_OFF  (HAL_GPIO_WritePin(LD2_GPIO_Port,LD2_Pin, GPIO_PIN_RESET))



#define PLAY_PRESSED (HAL_GPIO_ReadPin(BUTTON_PLAY_GPIO_Port, BUTTON_PLAY_Pin) == GPIO_PIN_SET)
#define REWIND_PRESSED (HAL_GPIO_ReadPin(BUTTON_REWIND_GPIO_Port, BUTTON_REWIND_Pin) == GPIO_PIN_SET)
#define SKIP_PRESSED (HAL_GPIO_ReadPin(BUTTON_SKIP_GPIO_Port, BUTTON_SKIP_Pin) == GPIO_PIN_SET)
#define VOL_DOWN_PRESSED (HAL_GPIO_ReadPin(BUTTON_VOL_DOWN_GPIO_Port, BUTTON_VOL_DOWN_Pin) == GPIO_PIN_SET)
#define VOL_UP_PRESSED   (HAL_GPIO_ReadPin(BUTTON_VOL_UP_GPIO_Port, BUTTON_VOL_UP_Pin) == GPIO_PIN_SET)
#define SCROLL_BUTTON_PRESSED (HAL_GPIO_ReadPin(BUTTON_SCROLL_GPIO_Port, BUTTON_SCROLL_Pin) == GPIO_PIN_SET)

#define SCROLL_KEY_HOLD_TIME 1000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

CRC_HandleTypeDef hcrc;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef hlpuart1;

SAI_HandleTypeDef hsai_BlockA1;
DMA_HandleTypeDef hdma_sai1_a;

SD_HandleTypeDef hsd1;
DMA_HandleTypeDef hdma_sdmmc1_rx;
DMA_HandleTypeDef hdma_sdmmc1_tx;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
/*
  _____  _____  _______      __       _______ ______   __      __     _____  _____          ____  _      ______  _____ 
 |  __ \|  __ \|_   _\ \    / /   /\ |__   __|  ____|  \ \    / /\   |  __ \|_   _|   /\   |  _ \| |    |  ____|/ ____|
 | |__) | |__) | | |  \ \  / /   /  \   | |  | |__      \ \  / /  \  | |__) | | |    /  \  | |_) | |    | |__  | (___   
 |  ___/|  _  /  | |   \ \/ /   / /\ \  | |  |  __|      \ \/ / /\ \ |  _  /  | |   / /\ \ |  _ <| |    |  __|  \___ \  
 | |    | | \ \ _| |_   \  /   / ____ \ | |  | |____      \  / ____ \| | \ \ _| |_ / ____ \| |_) | |____| |____ ____) | 
 |_|    |_|  \_\_____|   \/   /_/    \_\|_|  |______|      \/_/    \_\_|  \_\_____/_/    \_\____/|______|______|_____/ 
                                                     
                                                             
*/

// Required when FF_MULTI_PARTITION = 1
// Maps logical volume 0 to physical drive 0, partition 1

// Add these globals at the top
volatile uint32_t ping_count = 0;
volatile uint32_t pong_count = 0;
volatile uint32_t ping_missed = 0;
volatile uint32_t pong_missed = 0;
volatile uint8_t s_fill_ping = 0;
volatile uint8_t s_fill_pong = 0;

//Volume Flag
volatile bool g_vol_up_pressed = false;
volatile bool g_vol_down_pressed = false;

// Volume Buttons
ButtonState vol_up_btn;
ButtonState vol_down_btn;


Event_t input_event;
volatile State_t currentState = STATE_HOME;

/* GPIO Inputs */
uint8_t input_register;
#define SCROLL_KEY_BIT 3
#define REWIND_KEY_BIT 2
#define PLAY_KEY_BIT 1
#define SKIP_KEY_BIT 0

uint8_t scroll_register;
#define SCROLL_FLAG_BIT 1
#define CLOCKWISE_SCROLL_BIT 0
#define ANTICLOCKWISE_SCROLL_BIT 2
bool scroll_key_pressed = false;
volatile bool scroll_key_held = false;

static uint32_t phase = 0;
//Global Variables
uint8_t timer_length=0;
uint8_t time_elapsed=0;

volatile bool scroll_key_release_pending = false;

static bool test_refresh = false;
static uint8_t draw_state = 0;
static uint32_t button_hold_time = 0;
//Initialisation Code
uint8_t last_state = 0;
uint8_t current_state = 0;
uint8_t next_state = 0;
bool down_button_last = false;

      bool setting_timer = false;
// SDMMC
FATFS MyFatFS; 
FIL MyFile;    
FRESULT res; 
UINT bw;  
uint32_t byteswritten;

// Audio
#define AUDIO_BUF_SIZE 8192 
#define READBUF_SIZE 4096
int16_t audio_buffer[AUDIO_BUF_SIZE]; // The main buffer
uint8_t playing = 0;                   // State flag
UINT bytes_read;                       // To track SD card progress
uint8_t readBuf[READBUF_SIZE]; // Buffer for encoded MP3 data
int16_t outBuf[2 * 1152];      // Buffer for decoded PCM data
uint32_t adc_buffer[1];
uint8_t timer_ready = 0;
uint8_t last_button_state = 0; 

// Display
uint8_t music_option = 0;
uint16_t gpio_pin = 0;
uint32_t duration;
uint8_t button_scroll_flag = 0;


// Miscellaneous (Buttons, Sensors, States etc.)
// All Inputs



uint8_t previous_volume = 0;
volatile int8_t scroll_value = 0;
int8_t previous_scroll_value = 0;
uint8_t refresh_needed = 0;
bool scroll_button_hold =false;

uint8_t option = 0;
GPIO_PinState s1, s2 = GPIO_PIN_RESET;

  menu_screen main_menu[] = {
    {"Music", STATE_MUSIC},
    {"Timer", STATE_TIMER},
    {"Alarm", STATE_ALARM},
    {"Settings", STATE_SETTINGS}
  };

  menu_screen timer_menu[] = {
    {"Back", STATE_HOME},
    {"Preset Timers", STATE_HOME},
    {"Set Timer", STATE_TIMER_SET}
  };

  menu_screen timer_preset_menu[] = {
    {"Back", STATE_HOME},
    {"5 mins", STATE_HOME},
    {"10 mins",STATE_HOME}
  };

  // menu_screen alarm_menu[] = {
  //   {"Back", MAIN_MENU},
  //   {"Preset Alarms", PRESET_ALARM_MENU},
  //   {"Set Alarm", SET_ALARM_MENU}
  // };

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
static void MX_CRC_Init(void);
static void MX_TIM2_Init(void);
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
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        time_elapsed++;
        if ((time_elapsed / 60) >= timer_length) {
            HAL_TIM_Base_Stop_IT(&htim2);
            currentState = STATE_TIMER_DONE;
            refresh_needed = 1;
        }
    }
}

void Check_Scroll_Key_Hold() {
    if (scroll_key_release_pending) {
        scroll_key_release_pending = false;
        button_hold_time = __HAL_TIM_GET_COUNTER(&htim1);
        HAL_TIM_Base_Stop(&htim1);
        scroll_key_held = false;
        if (button_hold_time >= SCROLL_KEY_HOLD_TIME) {
            input_event = EVENT_SCROLL_HOLD;
        } else {
            input_event = EVENT_SCROLL_PRESS;
        }
    }
}

void Check_Scroll() {
  if (scroll_register & (1 << SCROLL_FLAG_BIT)) {
    if (scroll_register & (1 << CLOCKWISE_SCROLL_BIT)) {
      input_event = EVENT_SCROLL_CLOCKWISE;
    }
    if (scroll_register & (1 << ANTICLOCKWISE_SCROLL_BIT)) {
      input_event = EVENT_SCROLL_ANTICLOCKWISE;
    }
    scroll_register &= ~(0b00000111); // Clear scroll direction bits and flag
  }
}

// void Check_Volume() {
//   if (music_volume > 100) music_volume = 100;
//   else if (music_volume < 0) music_volume = 0;
// }

void Check_Music_Buttons() {
  if ((input_register & (1 << PLAY_PRESSED)) || (input_register & (1 << SKIP_PRESSED)) || (input_register & (1 << REWIND_PRESSED))) {
    //Play Song
    DrawTextToScreen("would play song");
  }
}
// }

void UI_State_Machine() {
  if (input_event == NONE) return;
  switch (currentState) {
    case STATE_HOME:
      if (input_event == EVENT_SCROLL_PRESS) {
        currentState = main_menu[option-1].next_screen;
        refresh_needed=1;
        option=1;
      }
      if (input_event == EVENT_SCROLL_CLOCKWISE) {option++;refresh_needed=1;}
      if (input_event == EVENT_SCROLL_ANTICLOCKWISE) {option--;refresh_needed=1;}
      break;
    case STATE_TIMER:
      if (input_event == EVENT_SCROLL_PRESS) {
        currentState = timer_menu[option-1].next_screen;
        refresh_needed=1;
        option=1;
      }
      if (input_event == EVENT_SCROLL_CLOCKWISE) {option++;refresh_needed=1;}
      if (input_event == EVENT_SCROLL_ANTICLOCKWISE) {option--;refresh_needed=1;}
      if (input_event == EVENT_SCROLL_HOLD) {
        currentState = STATE_HOME;
        refresh_needed=1;
        option=1;
      }
      break;
    case STATE_TIMER_SET:
      if (input_event == EVENT_SCROLL_CLOCKWISE && !setting_timer) {option++;refresh_needed=1;}
      else if (input_event == EVENT_SCROLL_ANTICLOCKWISE && !setting_timer) {option--;refresh_needed=1;}

      else if (option==1 && input_event == EVENT_SCROLL_PRESS) {currentState=STATE_HOME;refresh_needed=1;option=1;}

      else if (option == 2 && !setting_timer && input_event == EVENT_SCROLL_PRESS) {
        setting_timer = true;
      refresh_needed=1;
      }
      else if (option == 2 && setting_timer && input_event == EVENT_SCROLL_CLOCKWISE) {
        timer_length++;
        refresh_needed=1;
      }
      else if (option == 2 && setting_timer && input_event == EVENT_SCROLL_ANTICLOCKWISE) {
        timer_length--;        
        refresh_needed=1;
      }
      else if (option == 2 && setting_timer && input_event == EVENT_SCROLL_PRESS) {
        setting_timer = false;
        refresh_needed=1;
      }
      else if (option == 3 && input_event == EVENT_SCROLL_PRESS) {
        HAL_TIM_Base_Start_IT(&htim2);
        currentState = STATE_HOME;
        refresh_needed = 1;
      }

      else if (input_event == EVENT_SCROLL_HOLD) {
        
        currentState = STATE_HOME;
            setting_timer = false;  // add this
        refresh_needed=1;
        option=1;
      }
      break;
    case STATE_TIMER_DONE:
      if (input_event == EVENT_SCROLL_PRESS) {
        currentState = STATE_HOME;
        refresh_needed=1;
        option=1;
      }
    default:
      break;
  }
  input_event = NONE;
  if (option > 4) option = 1;
  if (option < 1) option = 4;
  if (refresh_needed) {
    refresh_needed=0;
    switch (currentState) {
      case STATE_HOME: LD3_OFF; DisplayOptions(option,main_menu[0].name, main_menu[1].name, main_menu[2].name, main_menu[3].name); break;
      case STATE_TIMER: DisplayOptionsThree(option, timer_menu[0].name, timer_menu[1].name, timer_menu[2].name); break;
      case STATE_TIMER_SET: timerSetUI(option, timer_length, setting_timer); break;
      case STATE_TIMER_DONE: LD3_ON; DrawTextToScreen("Timer Done"); break;
      default: break;
    } 
  }
}

void timerCheck() {
  if (currentState == STATE_TIMER_DONE && refresh_needed) {
        refresh_needed = 0;
        LD3_ON;
        DrawTextToScreen("Timer Done");
    }
}

/* Testing and Debugging */

void TestInputsOnly() {
  if (PLAY_PRESSED || REWIND_PRESSED || SKIP_PRESSED || VOL_DOWN_PRESSED || VOL_UP_PRESSED || SCROLL_BUTTON_PRESSED) HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
  else HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
}

void TestInputsAndDisplay() {
  ssd1306_Fill(Black);
  #ifdef SSD1306_INCLUDE_FONT_7x10
  ssd1306_SetCursor(2, 25);

  if (PLAY_PRESSED) DrawTextToScreen("Play");
  else if (REWIND_PRESSED) ssd1306_WriteString("Rewind", Font_7x10, White);
  else if (SKIP_PRESSED) ssd1306_WriteString("Skip", Font_7x10, White);
  else if (VOL_DOWN_PRESSED) ssd1306_WriteString("Volume Down", Font_7x10, White);
  else if (VOL_UP_PRESSED) ssd1306_WriteString("Volume Up", Font_7x10, White);
  else if (SCROLL_BUTTON_PRESSED) ssd1306_WriteString("Scroll Pressed", Font_7x10, White);
  else DrawTextToScreen("No Input");
  
  #endif
  ssd1306_UpdateScreen();
}

void Test_Scroll() {
  DrawTextToScreen("Scroll Test");
  if (scroll_register & (1 << SCROLL_FLAG_BIT)) {
    if (scroll_register & (1 << CLOCKWISE_SCROLL_BIT)) {
      DrawTextToScreen("Clockwise");
        HAL_Delay(1500);
    }
    if (scroll_register & (1 << ANTICLOCKWISE_SCROLL_BIT)) {
      DrawTextToScreen("Anticlockwise");
      HAL_Delay(1500);
    }
    scroll_register &= ~(0b00000111); // Clear scroll direction bits and flag
  } 
}

void Test_Scroll_Key_Hold() {
  char text_time[32];

  if ((input_register & (1 << SCROLL_KEY_BIT)) && !scroll_key_held) {
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    HAL_TIM_Base_Start(&htim1);
    scroll_key_held = true;
        draw_state=1;
    test_refresh=true;
  }
  if (!(input_register & (1 << SCROLL_KEY_BIT)) && scroll_key_held) {
    button_hold_time = __HAL_TIM_GET_COUNTER(&htim1);
    HAL_TIM_Base_Stop(&htim1);
            draw_state=2;
    test_refresh=true;
    //input_register &= ~(1 << SCROLL_KEY_BIT); // Zero this bit
    if (button_hold_time >= SCROLL_KEY_HOLD_TIME) {
input_event = EVENT_SCROLL_HOLD;
    }
    else {
input_event = EVENT_SCROLL_PRESS;
    }
    // input_register &= ~(1 << SCROLL_KEY_BIT);
    scroll_key_held = false;
  }
  
  if (input_event == EVENT_SCROLL_HOLD) {
    LD2_ON; LD3_OFF;
  } else if (input_event == EVENT_SCROLL_PRESS) {
    LD3_ON; LD2_OFF;
  }

if (test_refresh){
  test_refresh=false;
  switch (draw_state) {
    case 0:
        DrawTextToScreen("Scroll Key Test"); break;
        case 1:
        DrawTextToScreen("Starting Timer"); break;
    case 2:
      sprintf(text_time,"Held for : %lu", button_hold_time);
      DrawTextToScreen(text_time);
      HAL_Delay(5000);
                  draw_state = 0;  // ← Return to idle state after displaying result
      break;
    default: break;
  }}
}

void TestSound(void) {
    DrawTextToScreen("Playing wave now");
    static uint8_t started = 0;
    if (started) return;  // Only initialize once
    
    for (int i = 0; i < AUDIO_BUF_SIZE; i++) {
        // Proper signed 16-bit square wave, interleaved L+R
        int16_t sample = ((i / 50) % 2 == 0) ? 10000 : -10000;
        ((int16_t*)audio_buffer)[i] = sample;
    }
    
    HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t*)audio_buffer, AUDIO_BUF_SIZE);
    started = 1;
}

void TestMusicWav() {
  res = f_mount(&MyFatFS, (TCHAR const*)SDPath, 1);
if (res == FR_OK) {
    // 2. Open the audio file
    res = f_open(&MyFile, "EQ.wav", FA_READ); 
    
    if (res == FR_OK) {
        // 3. Skip WAV header (44 bytes)
        f_lseek(&MyFile, 44);
        
        // 4. Initial Buffer Fill (Read bytes, but fill half the samples)
        // We read AUDIO_BUF_SIZE * 2 because each sample is 2 bytes (uint16_t)
        res = f_read(&MyFile, audio_buffer, AUDIO_BUF_SIZE * 2, &bytes_read);
        
        if (res == FR_OK && bytes_read > 0) {
            ssd1306_SetCursor(2, 0);
            ssd1306_WriteString("Playing Music...", Font_7x10, White);
            ssd1306_UpdateScreen();

            // 5. Start SAI DMA (SAI expects number of samples, not bytes)
            HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t *)audio_buffer, AUDIO_BUF_SIZE);
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
void TestSD() {
//   uint8_t detected = BSP_SD_IsDetected();
// uint8_t bsp = BSP_SD_Init();
// char buff[32];
// sprintf(buff, "Det:%d BSP:%d", detected, bsp);
// DrawTextToScreen(buff);
// HAL_Delay(3000);
//   // --- RAW SD DIAGNOSTIC ---
//     HAL_SD_CardInfoTypeDef cardInfo;
//     HAL_StatusTypeDef sd_status = HAL_SD_GetCardInfo(&hsd1, &cardInfo);
//     char buf[40];
//     sprintf(buf, "SD:%d Blk:%lu", sd_status, cardInfo.BlockNbr);
//     DrawTextToScreen(buf);
//     HAL_Delay(3000);

//     // Read sector 0 (MBR) raw and show first 4 bytes
//     uint8_t sector[512];
//     HAL_StatusTypeDef read_status = HAL_SD_ReadBlocks(&hsd1, sector, 0, 1, 1000);
//     sprintf(buf, "Rd:%d %02X%02X%02X%02X", read_status,
//             sector[0], sector[1], sector[2], sector[3]);
//     DrawTextToScreen(buf);
//     HAL_Delay(3000);

//     // Show the MBR signature bytes (should be 55 AA at offset 510-511)
//     sprintf(buf, "MBR sig: %02X %02X", sector[510], sector[511]);
//     DrawTextToScreen(buf);
//     HAL_Delay(3000);
//     // --- END DIAGNOSTIC ---
//  BSP_SD_Init();          // force SD hardware init first
//     HAL_Delay(100);         // let card settle

static FATFS s_fatfs;
f_mount(&s_fatfs, "0:", 1);
DIR dir;
FILINFO fno;
f_opendir(&dir, "0:");
while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) {
    DrawTextToScreen(fno.fname);
    HAL_Delay(2000);
}
f_closedir(&dir);

  // UINT bw; // Variable to store how many bytes were actually written
  // res = f_mount(&MyFatFS, (TCHAR const *)SDPath, 1); // res = result

  // if (res == FR_OK) {
  //   if (res == FR_OK || res == FR_EXIST) {
  //     res = f_open(&MyFile, "poo.txt", FA_WRITE | FA_CREATE_ALWAYS);
  //     if (res == FR_OK) {
  //       f_write(&MyFile, "Gurt: Yo!", 9, &bw);
  //       DrawTextToScreen("Gurt:Yo!");
  //       f_close(&MyFile);
  //       }
  //     else {
  //       DrawTextToScreen("Open Failure");
  //     }
  //   }
  //   f_mount(NULL, "", 0);
  // }
  // else {
  //   char err[20];
  //   sprintf(err, "Res Error: %d", res);
  //   DrawTextToScreen(err);
  // }
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
  MX_CRC_Init();
  MX_TIM2_Init();
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
  //HAL_TIM_Base_Start_IT(&htim1);

  // Display Initialisation
  ssd1306_Init();
  //HAL_Delay(500);
  //TestSD();
  //MusicPlayer_Init(); MusicPlayer_Play("EQUIL.mp3");
  //TestSound();
  TestMusicWav();
  // Input_Init(&vol_up_btn, BUTTON_VOL_UP_GPIO_Port, BUTTON_VOL_UP_Pin);
  // Input_Init(&vol_down_btn, BUTTON_VOL_DOWN_GPIO_Port, BUTTON_VOL_DOWN_Pin);
  
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
    /* USER CODE - temporary diagnostic */
    
    Check_Scroll();
    MusicPlayer_Process();
    // 3. DEBUG: Slow down the screen updates significantly
    // static uint32_t last_print = 0;
    // if (HAL_GetTick() - last_print > 500) { // Update twice a second, not constantly
    //     last_print = HAL_GetTick();
    //     char dbg[32];
    //     If ping_missed or pong_missed is climbing, SDMMC is too slow
    //     sprintf(dbg, "P:%lu M:%lu", ping_count, ping_missed);
    //     DrawTextToScreen(dbg); 
    // }

    /* Debugging Functions */
    //TestInputs();
    //TestInputsAndDisplay();
    //TestSoundandDisplay();
    //Test_Scroll();
    //Test_Scroll_Key_Hold();
    //if (input_register & (1 << SCROLL_KEY_BIT)) {LD2_ON;} else {LD2_OFF;}

    /*Controller - View Inputs and change variables accordingly*/


    if (g_vol_down_pressed) {
    g_vol_down_pressed = false;
    ButtonEvent event = Input_Update(&vol_down_btn);
    if (event == BUTTON_EVENT_SHORT_PRESS)
        AudioControl_AdjustVolume(-5);
    else if (event == BUTTON_EVENT_LONG_PRESS)
        AudioControl_AdjustVolume(-20);
    }

    Check_Music_Buttons();
    Check_Scroll_Key_Hold();
    UI_State_Machine();
    timerCheck();
    /* Draw to Screen */
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
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 71;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_SDMMC1
                              |RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLLSAI1;
  PeriphClkInit.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_PLLSAI1;
  PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_MSI;
  PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
  PeriphClkInit.PLLSAI1.PLLSAI1N = 24;
  PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_48M2CLK|RCC_PLLSAI1_ADC1CLK;
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
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

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
  hi2c1.Init.Timing = 0x00701137;
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
  hlpuart1.Init.BaudRate = 115200;
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
  hsai_BlockA1.SlotInit.FirstBitOffset = 0;
hsai_BlockA1.SlotInit.SlotSize = SAI_SLOTSIZE_16B; // Force 32-bit slots
hsai_BlockA1.SlotInit.SlotNumber = 2;
hsai_BlockA1.SlotInit.SlotActive = SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1;
HAL_SAI_Init(&hsai_BlockA1);
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
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 14200;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(BUTTON_SCROLL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai) {
    LD3_ON; // If this lights, DMA is faulting
}

// void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai) {
//   if (hsai->Instance == SAI1_Block_A) {
//   MusicPlayer_SAI_HalfCpltCallback();
//   }  
//     if (s_fill_ping) ping_missed++;  // flag wasn't cleared yet = missed!
// s_fill_ping = 1;
// ping_count++;
// }

// void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai) {
//       if (hsai->Instance == SAI1_Block_A) {MusicPlayer_SAI_CpltCallback();}
//     if (s_fill_pong) pong_missed++;  // flag wasn't cleared yet = missed!
// s_fill_pong = 1;
// pong_count++;
// }

// This handles the "Ping" (First Half)
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai) {
    UINT br;
    // Fill the first half of the buffer (index 0 to 4095)
    // Size is (AUDIO_BUF_SIZE / 2) * 2 bytes because each sample is 16-bit
    f_read(&MyFile, &audio_buffer[0], AUDIO_BUF_SIZE, &br);
    
    if (br < AUDIO_BUF_SIZE) {
        // End of file reached: Close or loop
        f_lseek(&MyFile, 44); 
    }
}

// This handles the "Pong" (Second Half)
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai) {
    UINT br;
    // Fill the second half of the buffer (index 4096 to 8191)
    f_read(&MyFile, &audio_buffer[AUDIO_BUF_SIZE / 2], AUDIO_BUF_SIZE, &br);
    
    if (br < AUDIO_BUF_SIZE) {
        // End of file reached: Close or loop
        f_lseek(&MyFile, 44);
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == SCROLL_S1_Pin) {
    scroll_register |= (1 << SCROLL_FLAG_BIT);
    scroll_register |= (HAL_GPIO_ReadPin(SCROLL_S2_GPIO_Port, SCROLL_S2_Pin)) ? (1 << ANTICLOCKWISE_SCROLL_BIT) : (1 << CLOCKWISE_SCROLL_BIT);
  }
  if (GPIO_Pin == BUTTON_VOL_UP_Pin) {
    g_vol_up_pressed = true;
  }
  if (GPIO_Pin == BUTTON_VOL_DOWN_Pin) {
      g_vol_down_pressed = true;
  }

  if (GPIO_Pin == BUTTON_SCROLL_Pin) {
    if (HAL_GPIO_ReadPin(BUTTON_SCROLL_GPIO_Port, BUTTON_SCROLL_Pin) == GPIO_PIN_SET) {
        input_register |= (1 << SCROLL_KEY_BIT);
        __HAL_TIM_SET_COUNTER(&htim1, 0);
        HAL_TIM_Base_Start(&htim1);
        scroll_key_held = true;
    } else {
        input_register &= ~(1 << SCROLL_KEY_BIT);
        scroll_key_release_pending = true;  // flag the release
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
