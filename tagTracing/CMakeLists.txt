cmake_minimum_required(VERSION 3.0.0)

project(rhoTagTracing   C)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_C_FLAGS " -g -Wall -std=c99 ")

# find the lib 
find_library(SET_LIBRARY set HINTS ../set)

# set the source files 
set(SOURCES tagTracing.c tagTracing.h)


# executable 
add_executable(rhoTagTracing rhoPollard_tagTracing.c ${SOURCES}) 

# includes
target_include_directories(rhoTagTracing PUBLIC ../set)

# library linking 
target_link_libraries(rhoTagTracing PUBLIC gmp pthread)
target_link_libraries(rhoTagTracing PUBLIC ${SET_LIBRARY})
