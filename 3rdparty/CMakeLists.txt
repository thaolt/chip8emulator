cmake_minimum_required(VERSION 2.8)

project(3rdparty)

add_subdirectory(termbox)
add_subdirectory(log.c)

option(TINYCTHREAD_DISABLE_TESTS "Disable TinyCThread unit tests" ON)
add_subdirectory(tinycthread)

option(BUILD_EXAMPLE_PROGRAMS "Build example programs" OFF)
option(BUILD_TESTS "Build tests" OFF)
add_subdirectory(libsoundio)


add_subdirectory(libucl)
