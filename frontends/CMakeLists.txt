cmake_minimum_required(VERSION 2.8)

project(frontends)

if (UNIX)
add_subdirectory(chip8emu-termbox)
add_subdirectory(chip8emu-cdk)
endif (UNIX)
add_subdirectory(chip8emu-sdl2)


