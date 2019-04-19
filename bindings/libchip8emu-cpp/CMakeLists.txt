cmake_minimum_required(VERSION 2.8)

project(chip8emu-cpp)

add_library(${PROJECT_NAME} "chip8emu.cpp" "chip8emu.hpp")
