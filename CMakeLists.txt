cmake_minimum_required(VERSION 3.15)
project(MyCW2)

set(CMAKE_CXX_STANDARD 14)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

SET(CMAKE_CXX_COMPILER mpicxx)
SET(CMAKE_C_COMPILER  mpicc)

add_executable(MyCW2 main.cpp)