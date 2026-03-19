cmake_minimum_required(VERSION 3.22)

# Test executable for input module
add_executable(test_input
    test_input.c
    ${CMAKE_SOURCE_DIR}/Core/Src/input.c
    ${CMAKE_SOURCE_DIR}/Tests/vendor/Unity/src/unity.c
)

# Include paths
target_include_directories(test_input PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/Inc
    ${CMAKE_SOURCE_DIR}/Tests/vendor/Unity/src
)

# Register with CTest
add_test(NAME test_input COMMAND test_input)