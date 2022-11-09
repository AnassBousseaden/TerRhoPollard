cmake_minimum_required(VERSION 3.0.0)

project(rho C)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_C_FLAGS " -g -Wall -std=c99 ")

# executable 
add_executable(rho rho.c)

# library linking 
target_link_libraries(rho PUBLIC gmp)
