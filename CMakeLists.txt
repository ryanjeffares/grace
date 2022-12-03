cmake_minimum_required(VERSION 3.8)
project(grace)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(deps/dyncall dyncall_build)
add_subdirectory(src grace)