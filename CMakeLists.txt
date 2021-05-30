cmake_minimum_required(VERSION 2.8)
project(life)

# Set compiler flags for build types
if (WIN32)
set(CMAKE_CXX_FLAGS "-Wall -std=c++17 -static-libgcc -static-libstdc++")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -std=c++17 -static-libgcc -static-libstdc++")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -std=c++17 -static-libgcc -static-libstdc++")
else()
set(CMAKE_CXX_FLAGS "-Wall -std=c++17")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -std=c++17")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -std=c++17")
endif(WIN32)

# Include custom cmake modules used to search for SDL libraries
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2)

# Find SDL libraries
find_package(SDL2 REQUIRED)

add_executable(
    ${PROJECT_NAME}
    life.cpp
    )

target_link_libraries(${PROJECT_NAME} SDL2::Main)
