cmake_minimum_required(VERSION 2.8)

project(logc)

add_library(${PROJECT_NAME} origin/src/log.c origin/src/log.h)
