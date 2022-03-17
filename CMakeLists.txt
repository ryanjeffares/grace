cmake_minimum_required(VERSION 3.8)
project(grace)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(FMT_HEADER_ONLY)

add_executable(grace src/main.cpp src/compiler.cpp src/scanner.cpp src/vm.cpp src/constant.cpp src/objects/grace_string.cpp)
