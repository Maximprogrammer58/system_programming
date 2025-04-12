cmake_minimum_required(VERSION 3.28.3)
project(lab_2)

set(CMAKE_CXX_STANDARD 20)

add_executable(signals signals.cpp)
add_executable(pipe pipe.cpp)
add_executable(queue queue.cpp)