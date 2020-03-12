cmake_minimum_required(VERSION 3.15)
project(untitled C)

set(CMAKE_C_CLANG_TIDY 99)

add_executable(untitled main.c squirrel.c squirrel.h sauirrel_queue.c sauirrel_queue.h prob.c)