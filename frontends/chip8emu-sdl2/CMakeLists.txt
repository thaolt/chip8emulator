cmake_minimum_required(VERSION 3.0.4)

project(chip8emu-sdl2)

add_executable(${PROJECT_NAME} "main.c")

find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})

string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES)

target_link_libraries(${PROJECT_NAME} chip8emu tinycthread logc ${SDL2_LIBRARIES})

