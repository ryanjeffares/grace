if (GRACE_BUILD_TARGET MATCHES "dll")
    add_library(grace SHARED
            dllmain.cpp
            compiler.cpp
            scanner.cpp
            value.cpp
            vm.cpp
            vm_register_natives.cpp
            objects/grace_dictionary.cpp
            objects/grace_exception.cpp
            objects/grace_instance.cpp
            objects/grace_iterator.cpp
            objects/grace_keyvaluepair.cpp
            objects/grace_list.cpp
            objects/grace_set.cpp
            objects/grace_range.cpp
            objects/object_tracker.cpp
            )
elseif (GRACE_BUILD_TARGET MATCHES "exe")
    add_executable(grace
            main.cpp
            compiler.cpp
            scanner.cpp
            value.cpp
            vm.cpp
            vm_register_natives.cpp
            objects/grace_dictionary.cpp
            objects/grace_exception.cpp
            objects/grace_instance.cpp
            objects/grace_iterator.cpp
            objects/grace_keyvaluepair.cpp
            objects/grace_list.cpp
            objects/grace_set.cpp
            objects/grace_range.cpp
            objects/object_tracker.cpp)
else ()
    message(FATAL_ERROR "GRACE_BUILD_TARGET must match 'exe' or 'dll'")
endif ()

target_include_directories(grace
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../deps/fmt/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../deps/dyncall/dyncall
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../deps/dyncall/dyncallback
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../deps/dyncall/dynload
        )

target_compile_definitions(grace PRIVATE FMT_HEADER_ONLY)
if (GRACE_PROFILE_OPS MATCHES "ON")
    target_compile_definitions(grace PRIVATE GRACE_PROFILE_OPS)
endif ()

set_target_properties(grace PROPERTIES
        CXX_STANDARD 20
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BUILD_TYPE}
        )

if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(grace PRIVATE GRACE_DEBUG)
endif ()

if (MSVC)
    target_compile_definitions(grace PRIVATE GRACE_MSC)
    target_compile_options(grace PRIVATE /W4 /WX /external:I ${CMAKE_CURRENT_SOURCE_DIR}/../deps/fmt/include /external:W0 /external:templates-)
    target_link_libraries(grace
            PRIVATE ../dyncall_build/dyncall/${CMAKE_BUILD_TYPE}/dyncall_s
            PRIVATE ../dyncall_build/dynload/${CMAKE_BUILD_TYPE}/dynload_s
            PRIVATE ../dyncall_build/dyncallback/${CMAKE_BUILD_TYPE}/dyncallback_s
            )
else ()
    target_compile_definitions(grace PRIVATE GRACE_GCC_CLANG)
    target_compile_options(grace PRIVATE -Wall -Wextra -Wpedantic -Werror -Wconversion)
    target_link_libraries(grace
            PRIVATE pthread
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../build/dyncall_build/dyncall/libdyncall_s.a
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../build/dyncall_build/dynload/libdynload_s.a
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../build/dyncall_build/dyncallback/libdyncallback_s.a
            PRIVATE ${CMAKE_DL_LIBS}
            )
endif ()

install(TARGETS grace RUNTIME DESTINATION bin)

if (MSVC)
    install(DIRECTORY ../std/ DESTINATION std/)
else ()
    install(DIRECTORY ../std/ DESTINATION share/grace/std)
endif ()
