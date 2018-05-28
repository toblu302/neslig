cmake_minimum_required(VERSION 3.5.1)
project(NESlig)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

#Set flags
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

#Include files and dependencies
find_package(SDL2 REQUIRED)
include_directories(${SDL_INCLUDE_DIRS})
include_directories(src)
file(GLOB SOURCES "src/*.cpp")

#Compiel
add_executable(NESlig ${SOURCES})
target_link_libraries(NESlig ${SDL2_LIBRARIES})
