cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

set(TARGET "amouse")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(AMOUSE_PATH ${PROJECT_SOURCE_DIR})

project(amouse)

# Initialize the SDK
pico_sdk_init()

add_executable(amouse
  	amouse.c ../shared/mouse.c ../shared/utils.c include/serial.c include/usb.c
        )

target_include_directories(amouse PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Pull in our pico_stdlib which pulls in commonly used features, also tinyUSB for HID
target_link_libraries(amouse pico_stdlib tinyusb_host tinyusb_board)

include_directories(include/ ../lib/)
link_directories(include/ ../lib/)

# create map/bin/hex file etc.
pico_add_extra_outputs(amouse)

# add url via pico_set_program_url
pico_set_program_name(${TARGET} "amouse") 
pico_set_program_description(${TARGET} "Anachro Mouse, a usb to serial mouse adaptor.")
pico_set_program_url(${TARGET} "https://github.com/Aviancer/amouse/tree/HEAD")
