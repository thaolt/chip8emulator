cmake_minimum_required(VERSION 2.8)

project(chip8emu-termbox)

add_executable(${PROJECT_NAME} "main.c")

target_link_libraries(${PROJECT_NAME} chip8emu tinycthread termbox logc libsoundio_static ${LIBSOUNDIO_LIBS} ${LIBM})
