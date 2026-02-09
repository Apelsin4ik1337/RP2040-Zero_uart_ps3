cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk)

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(ps3_uart_bridge C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(ps3_uart_bridge
    main.c
    usb_descriptors.c
)

target_include_directories(ps3_uart_bridge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ps3_uart_bridge
    pico_stdlib
    pico_unique_id
    hardware_uart
    hardware_irq
    hardware_gpio
    tinyusb_device
    tinyusb_board
)

pico_enable_stdio_usb(ps3_uart_bridge 0)
pico_enable_stdio_uart(ps3_uart_bridge 0)

pico_add_extra_outputs(ps3_uart_bridge)