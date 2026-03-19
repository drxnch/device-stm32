cmake_minimum_required(VERSION 3.22)

# Test executable for input module
add_executable(test_input
    test_input.c
    ${CMAKE_SOURCE_DIR}/Core/Src/input.c
    ${CMAKE_SOURCE_DIR}/Tests/vendor/Unity/src/unity.c
    ${CMAKE_SOURCE_DIR}/Tests/mocks/mock_stm32l4xx_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/Tests/mocks/mock_stm32l4xx_hal.c
    ${CMAKE_SOURCE_DIR}/Tests/vendor/CMock/src/cmock.c
)

# Include paths
target_include_directories(test_input PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/Inc
    ${CMAKE_SOURCE_DIR}/Tests/vendor/Unity/src
    ${CMAKE_SOURCE_DIR}/Tests/vendor/CMock/src
    ${CMAKE_SOURCE_DIR}/Tests/mocks
    ${CMAKE_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32L4xx/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
)

# Register with CTest
add_test(NAME test_input COMMAND test_input)

target_compile_definitions(test_input PRIVATE
    STM32L496xx
)