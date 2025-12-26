cmake_minimum_required(VERSION 3.16)
project(pmr_lab)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Основная программа
add_executable(pmr_main 
    src/main.cpp
    src/MapMemoryResource.cpp
)
target_include_directories(pmr_main PRIVATE include)

# GoogleTest через FetchContent
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Тесты
add_executable(pmr_tests 
    tests/tests.cpp
    src/MapMemoryResource.cpp
)
target_include_directories(pmr_tests PRIVATE include)

# Используем псевдонимы таргетов вместо прямых библиотек
target_link_libraries(pmr_tests PRIVATE 
    GTest::gtest_main  # Это включает и gtest тоже
)

include(GoogleTest)
gtest_discover_tests(pmr_tests)
