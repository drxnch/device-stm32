cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME Firmware)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_PROJECT_NAME Firmware)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

enable_language(C ASM)

if(NOT DEFINED STM32_TARGET_BUILD)
    enable_testing()
    add_subdirectory(tests)
else()
    add_subdirectory(cmake/stm32cubemx)
    add_compile_options(-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16)
    add_link_options(-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16)
    add_executable(${CMAKE_PROJECT_NAME})
    target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        Core/Src/ssd1306.c
        Core/Src/ssd1306_tests.c
        Core/Src/ssd1306_fonts.c
        Core/Src/display.c
    )
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        Core/Libs
    )
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE)
    list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
    target_link_libraries(${CMAKE_PROJECT_NAME}
        stm32cubemx
        "${CMAKE_CURRENT_SOURCE_DIR}/Core/Libs/lib_mp3decoder_cortex_m4_v2.2.0.a"
    )
endif()