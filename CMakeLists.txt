cmake_minimum_required(VERSION 2.8)

project(chip8emulator)

include_directories(${CMAKE_SOURCE_DIR}/libchip8emu)

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

add_subdirectory(3rdparty)
add_subdirectory(libchip8emu)
add_subdirectory(emulator)
add_subdirectory(emu-termbox)
