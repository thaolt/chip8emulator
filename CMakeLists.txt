cmake_minimum_required(VERSION 2.8)

project(chip8emulator)

include_directories(${CMAKE_SOURCE_DIR}/libchip8emu)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/termbox/origin/src)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/tinycthread/source)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/libsoundio/soundio)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/libsoundio/src)

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/lib)

add_subdirectory(3rdparty)
add_subdirectory(libchip8emu)
add_subdirectory(frontends)
add_subdirectory(bindings)
add_subdirectory(roms)
