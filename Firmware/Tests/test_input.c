#include "unity.h"
#include "input.h"
#include <stdbool.h>

void setUp(void) {}    // runs before each test
void tearDown(void) {} // runs after each test

void test_input_init_set_default_state(void) {
    ButtonState btn;
    Input_Init(&btn);

    TEST_ASSERT_FALSE(btn.debounced_state);
    TEST_ASSERT_FALSE(btn.last_raw_state);
    TEST_ASSERT_FALSE(btn.press_reported);
}

void test_input_update_set_button_event_short_press(void) {
    ButtonState btn;
    // Press the button
    Input_Update(&btn, true, 0);
    // Wait past debounce threshold
    Input_Update(&btn, true, 25);
    // Release the button
    Input_Update(&btn, false, 26);
    // Wait past debounce threshold after release
    ButtonEvent event = Input_Update(&btn, false, 51);
    TEST_ASSERT_EQUAL(BUTTON_EVENT_SHORT_PRESS, event);
}

void test_input_update_set_button_event_long_press(void) {
    ButtonState btn;

    Input_Update(&btn, true,0);
    Input_Update(&btn, true, 25);
    ButtonEvent event = Input_Update(&btn,true, 1005);

    TEST_ASSERT_EQUAL(BUTTON_EVENT_LONG_PRESS, event);

}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_input_init_set_default_state);
    RUN_TEST(test_input_update_set_button_event_short_press);
    RUN_TEST(test_input_update_set_button_event_long_press);
    return UNITY_END();
}