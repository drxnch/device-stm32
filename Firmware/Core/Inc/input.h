#pragma once // Can use instead of #ifndef

#include <stdint.h>
#include <stdbool.h>

#define DEBOUNCE_MS         20
#define LONG_PRESS_MS       800

typedef enum {
    BUTTON_EVENT_NONE,
    BUTTON_EVENT_SHORT_PRESS,
    BUTTON_EVENT_LONG_PRESS,
} ButtonEvent;

typedef struct {
    bool last_raw_state;
    bool debounced_state;
    uint32_t state_change_time_ms;
    bool press_reported;
} ButtonState;

void Input_Init(ButtonState *btn);
ButtonEvent Input_Update(ButtonState *btn, bool raw_pin_state, uint32_t current_time_ms);