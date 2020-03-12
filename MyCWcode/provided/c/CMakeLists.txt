cmake_minimum_required(VERSION 3.15)
project(untitled C)

set(CMAKE_C_STANDARD 99)

add_executable(untitled main.c squirrel.c squirrel.h sauirrel_queue.c sauirrel_queue.h prob.c)