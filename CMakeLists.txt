cmake_minimum_required(VERSION 3.28)
project(mysh C)

set(CMAKE_C_STANDARD 99)

enable_testing()

add_subdirectory(src)
add_subdirectory(test)