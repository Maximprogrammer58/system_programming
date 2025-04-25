cmake_minimum_required(VERSION 3.28)
project(lab_3)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

include_directories(include)

add_library(matrix_lib SHARED src/Matrix.cpp)
target_include_directories(matrix_lib PUBLIC include)

add_executable(mul_matrix mul_matrix.cpp)
target_link_libraries(mul_matrix matrix_lib)

add_executable(find_in_vector find_in_vector.cpp)
target_link_libraries(find_in_vector matrix_lib)

add_executable(queue queue.cpp)
target_link_libraries(queue matrix_lib)
