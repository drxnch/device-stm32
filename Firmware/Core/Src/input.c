#include "input.h"

void Input_Init(ButtonState *btn) {
    btn->last_raw_state = false;
    btn->debounced_state = false;
    btn->state_change_time_ms = 0;
    btn->press_reported = false;
}

ButtonEvent Input_Update(ButtonState *btn, bool raw_pin_state, uint32_t current_time_ms) {
    // If raw state changed, reset debounce timer
    if (raw_pin_state != btn->last_raw_state) {
        btn->last_raw_state = raw_pin_state;
        btn->state_change_time_ms = current_time_ms;
        return BUTTON_EVENT_NONE;
    }

    // Not enough time passed yet for debounce
    if ((current_time_ms - btn->state_change_time_ms) < DEBOUNCE_MS) {
        return BUTTON_EVENT_NONE;
    }

    // State is stable, update debounced state
    bool prev_debounced = btn->debounced_state;
    btn->debounced_state = raw_pin_state;

    // Button just released
    if (prev_debounced && !btn->debounced_state) {
        if (btn->press_reported) {
            // Long press was already reported on the way down
            btn->press_reported = false;
            return BUTTON_EVENT_NONE;
        }
        btn->press_reported = false;
        return BUTTON_EVENT_SHORT_PRESS;
    }

    // Button held down — check for long press
    if (btn->debounced_state && !btn->press_reported) {
        if ((current_time_ms - btn->state_change_time_ms) >= LONG_PRESS_MS) {
            btn->press_reported = true;
            return BUTTON_EVENT_LONG_PRESS;
        }
    }

    return BUTTON_EVENT_NONE;
}