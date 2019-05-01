cmake_minimum_required(VERSION 2.8)

project(chip8emu-termbox)

include_directories(${CMAKE_SOURCE_DIR}/3rdparty/Collections-C/src/include)

set(HEADER_FILES "tbui.h")

set(SOURCES
     "main.c" "tbui.c"
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADER_FILES})


set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)

target_link_libraries(${PROJECT_NAME} 
    chip8emu
    tinycthread
    termbox
    libsoundio_static ${LIBSOUNDIO_LIBS} 
    ${LIBM}
)


