cmake_minimum_required(VERSION 3.5.1)
project(NESlig)

#Set flags
set( CMAKE_CXX_FLAGS "-Wall -std=c++17 -O3 " )

#Include files and dependencies
find_package(SDL2 REQUIRED)
include_directories(${SDL_INCLUDE_DIRS})
include_directories(src)
file(GLOB SOURCES "src/*.c")

#Compiel
add_executable(NESlig ${SOURCES})
target_link_libraries(NESlig ${SDL2_LIBRARIES})
