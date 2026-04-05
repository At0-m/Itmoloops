cmake_minimum_required(VERSION 3.20)
project(itmoloops LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ITMOLOOPS_BUILD_TESTS "Build tests" OFF)

add_subdirectory(src/itmoloops)
add_subdirectory(bin)

if(ITMOLOOPS_BUILD_TESTS)
  add_subdirectory(tests)
endif()