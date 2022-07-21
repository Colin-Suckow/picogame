cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(picogame)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()


add_executable(picogame
picogame.c
)

target_sources(picogame PRIVATE
${CMAKE_CURRENT_LIST_DIR}/picogame.c
${CMAKE_CURRENT_LIST_DIR}/vga.c
)

add_compile_options(-Wall -Wextra -pedantic -Werror)

pico_generate_pio_header(picogame ${CMAKE_CURRENT_LIST_DIR}/hsync.pio)
pico_generate_pio_header(picogame ${CMAKE_CURRENT_LIST_DIR}/vsync.pio)
pico_generate_pio_header(picogame ${CMAKE_CURRENT_LIST_DIR}/rgb.pio)

target_link_libraries(picogame pico_stdlib hardware_pio hardware_dma)
pico_enable_stdio_usb(picogame 1)
pico_enable_stdio_uart(picogame 0)
pico_add_extra_outputs(picogame)
