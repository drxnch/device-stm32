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

Event_t input_event;
volatile State_t currentState = STATE_HOME;

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