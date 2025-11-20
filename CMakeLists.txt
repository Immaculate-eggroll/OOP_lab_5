cmake_minimum_required(VERSION 3.10)
project(OOP_lab_3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(oop_lab_3
    src/main.cpp
    src/MapMemoryResource.cpp
)
