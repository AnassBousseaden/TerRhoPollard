cmake_minimum_required(VERSION 3.0.0)

project(rhoPollard_rAddingWalk C)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_C_FLAGS " -g -Wall -std=c99 ")

# executable 
add_executable(rhoPollard_rAddingWalk rhoPollard_rAddingWalk.c)

# library linking 
target_link_libraries(rhoPollard_rAddingWalk PUBLIC gmp)
