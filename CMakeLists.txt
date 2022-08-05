cmake_minimum_required(VERSION 3.8)
project(grace)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(grace 
  src/main.cpp
  src/compiler.cpp
  src/scanner.cpp
  src/value.cpp
  src/vm.cpp
  src/vm_register_natives.cpp
  src/objects/grace_dictionary.cpp
  src/objects/grace_exception.cpp
  src/objects/grace_instance.cpp
  src/objects/grace_iterator.cpp
  src/objects/grace_keyvaluepair.cpp
  src/objects/grace_list.cpp
  src/objects/object_tracker.cpp
)

target_include_directories(grace PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/fmt/include)
target_compile_definitions(grace PRIVATE FMT_HEADER_ONLY)
set_target_properties(grace PROPERTIES
  CXX_STANDARD 20
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BUILD_TYPE}
)

set_target_properties(grace PROPERTIES CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")

if (CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_definitions(grace PRIVATE GRACE_DEBUG)
endif()

if(MSVC)
  target_compile_definitions(grace PRIVATE GRACE_MSC)
  target_compile_options(grace PRIVATE /W4 /WX /external:W0 /external:templates-)
else()
  target_compile_options(grace PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_link_libraries(grace pthread)
endif()

install(TARGETS grace DESTINATION bin)
install(DIRECTORY std DESTINATION std)
