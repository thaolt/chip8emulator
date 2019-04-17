cmake_minimum_required(VERSION 2.8)

project(termbox)

add_library(${PROJECT_NAME} origin/src/termbox.c origin/src/utf8.c)
