cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)
project(pico-ILI9341_parallel_16B C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()


add_executable(lcd
        lcd.c
        )

target_link_libraries(lcd pico_stdlib)
pico_add_extra_outputs(lcd)
