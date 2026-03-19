#include "unity.h"
#include "mock_stm32l4xx_hal_gpio.h"
#include "mock_stm32l4xx_hal.h"
#include "input.h"

static ButtonState btn;

void setUp(void) {
    mock_stm32l4xx_hal_gpio_Init();
    mock_stm32l4xx_hal_Init();
    Input_Init(&btn, GPIOC, GPIO_PIN_13);
}

void tearDown(void) {
    mock_stm32l4xx_hal_gpio_Verify();
    mock_stm32l4xx_hal_gpio_Destroy();
    mock_stm32l4xx_hal_Verify();
    mock_stm32l4xx_hal_Destroy();
}

void test_input_init_set_default_state(void) {
    TEST_ASSERT_FALSE(btn.debounced_state);
    TEST_ASSERT_FALSE(btn.last_raw_state);
    TEST_ASSERT_FALSE(btn.press_reported);
}

void test_input_update_set_button_event_short_press(void) {
    // Press the button - raw state changes
    HAL_GPIO_ReadPin_ExpectAndReturn(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_GetTick_ExpectAndReturn(0);
    Input_Update(&btn);

    // Wait past debounce - state stable
    HAL_GPIO_ReadPin_ExpectAndReturn(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_GetTick_ExpectAndReturn(25);
    Input_Update(&btn);

    // Release button - raw state changes
    HAL_GPIO_ReadPin_ExpectAndReturn(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GetTick_ExpectAndReturn(26);
    Input_Update(&btn);

    // Wait past debounce after release
    HAL_GPIO_ReadPin_ExpectAndReturn(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GetTick_ExpectAndReturn(51);
    ButtonEvent event = Input_Update(&btn);

    TEST_ASSERT_EQUAL(BUTTON_EVENT_SHORT_PRESS, event);
}

void test_input_update_set_button_event_long_press(void) {
    // Press the button
    HAL_GPIO_ReadPin_ExpectAndReturn(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_GetTick_ExpectAndReturn(0);
    Input_Update(&btn);

    // Wait past debounce
    HAL_GPIO_ReadPin_ExpectAndReturn(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_GetTick_ExpectAndReturn(25);
    Input_Update(&btn);

    // Hold past long press threshold
    HAL_GPIO_ReadPin_ExpectAndReturn(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_GetTick_ExpectAndReturn(900);
    ButtonEvent event = Input_Update(&btn);

    TEST_ASSERT_EQUAL(BUTTON_EVENT_LONG_PRESS, event);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_input_init_set_default_state);
    RUN_TEST(test_input_update_set_button_event_short_press);
    RUN_TEST(test_input_update_set_button_event_long_press);
    return UNITY_END();
}