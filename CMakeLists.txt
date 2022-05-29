cmake_minimum_required(VERSION 3.8)
project(grace)

add_executable(grace 
  src/main.cpp
  src/compiler.cpp
  src/scanner.cpp
  src/value.cpp
  src/vm.cpp
  src/vm_register_natives.cpp
  src/objects/grace_exception.cpp
  src/objects/grace_list.cpp
  src/objects/object_tracker.cpp
)

target_include_directories(grace PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/fmt/include)

set_property(TARGET grace PROPERTY EXPORT_COMPILE_COMMANDS ON)
set_property(TARGET grace PROPERTY CXX_STANDARD 20)
set_property(TARGET grace PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BUILD_TYPE})

if (NOT CMAKE_BUILD_TYPE MATCHES Release)
  add_compile_definitions(GRACE_DEBUG)
endif(NOT CMAKE_BUILD_TYPE MATCHES Release)

target_compile_definitions(grace PRIVATE FMT_HEADER_ONLY)

if(MSVC)
  target_compile_options(grace PRIVATE /W4 /WX)
else()
  target_compile_options(grace PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
