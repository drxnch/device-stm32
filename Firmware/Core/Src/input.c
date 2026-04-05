#include "input.h"

void Input_Init(ButtonState *btn, GPIO_TypeDef *port, uint16_t pin) {
    btn->port = port;
    btn->pin = pin;
    btn->last_raw_state = false;
    btn->debounced_state = false;
    btn->state_change_time_ms = 0;
    btn->press_reported = false;
}

ButtonEvent Input_Update(ButtonState *btn) {
    bool raw_pin_state = HAL_GPIO_ReadPin(btn->port, btn->pin) == GPIO_PIN_SET;
    uint32_t current_time_ms = HAL_GetTick();

    if (raw_pin_state != btn->last_raw_state) {
        btn->last_raw_state = raw_pin_state;
        btn->state_change_time_ms = current_time_ms;
        return BUTTON_EVENT_NONE;
    }

    if ((current_time_ms - btn->state_change_time_ms) < DEBOUNCE_MS) {
        return BUTTON_EVENT_NONE;
    }

    bool prev_debounced = btn->debounced_state;
    btn->debounced_state = raw_pin_state;

    if (prev_debounced && !btn->debounced_state) {
        if (btn->press_reported) {
            btn->press_reported = false;
            return BUTTON_EVENT_NONE;
        }
        btn->press_reported = false;
        return BUTTON_EVENT_SHORT_PRESS;
    }

    if (btn->debounced_state && !btn->press_reported) {
        if ((current_time_ms - btn->state_change_time_ms) >= LONG_PRESS_MS) {
            btn->press_reported = true;
            return BUTTON_EVENT_LONG_PRESS;
        }
    }

    return BUTTON_EVENT_NONE;
}

void Check_Music_Buttons() {
  if ((input_register & (1 << PLAY_PRESSED)) || (input_register & (1 << SKIP_PRESSED)) || (input_register & (1 << REWIND_PRESSED))) {
    //Play Song
    DrawTextToScreen("would play song");
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
