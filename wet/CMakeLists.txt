cmake_minimum_required(VERSION 3.15)
project(hw4)

set(CMAKE_CXX_STANDARD 11)

add_executable(hw4_1 main.cpp malloc_1.cpp)
add_executable(hw4_2 main.cpp malloc_2.cpp)
add_executable(hw4_3 main.cpp malloc_3.cpp)
add_executable(hw4_4 main.cpp malloc_4.cpp)
add_executable(hw4_test main_test.cpp malloc_3.cpp)
add_executable(hw4_test2 malloc3_srealloc_tests.cpp malloc_3.cpp)
add_executable(hw4_test3 tamuz_tests_hw4_malloc2.cpp malloc_2.cpp)