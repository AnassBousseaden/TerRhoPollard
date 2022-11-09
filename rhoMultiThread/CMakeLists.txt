cmake_minimum_required(VERSION 3.0.0)

project(rhoMultiThread C)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_C_FLAGS " -g -Wall -std=c99 ")

# find the lib 
find_library(SET_LIBRARY set HINTS ../set)

# executable 
add_executable(rhoMultiThread rhoPollard_Threads.c)

# includes
target_include_directories(rhoMultiThread PUBLIC ../set)

# library linking 
target_link_libraries(rhoMultiThread PUBLIC gmp pthread)
target_link_libraries(rhoMultiThread PUBLIC ${SET_LIBRARY})
