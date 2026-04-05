// Add these globals at the top
volatile uint32_t ping_count = 0;
volatile uint32_t pong_count = 0;
volatile uint32_t ping_missed = 0;
volatile uint32_t pong_missed = 0;
volatile uint8_t s_fill_ping = 0;
volatile uint8_t s_fill_pong = 0;

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