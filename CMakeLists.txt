cmake_minimum_required(VERSION 3.10)
project(rpi_gesture_recognition LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Opcja kompilacji bez GUI (użycie: cmake -DHEADLESS=ON ..)
option(HEADLESS "Build without OpenCV GUI windows" OFF)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

add_executable(gesture_app src/main.cpp)

target_link_libraries(gesture_app PRIVATE ${OpenCV_LIBS} Threads::Threads)

# Flagi optymalizacji pod Raspberry Pi 3B (ARM Cortex-A53)
target_compile_options(gesture_app PRIVATE -O3 -mcpu=cortex-a53 -Wall -Wextra)

if(HEADLESS)
    target_compile_definitions(gesture_app PRIVATE HEADLESS)
endif()