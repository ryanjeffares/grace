cmake_minimum_required(VERSION 3.8)
project(grace)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

link_libraries(fmt)
add_executable(grace src/main.cpp src/compiler.cpp src/scanner.cpp)
