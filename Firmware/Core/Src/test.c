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