# cod3pc - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(cod3pc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(COD3PC_SOURCES
    src/main.cpp
    src/runtime/entry_trace.cpp
    src/runtime/crash_trace.cpp
    src/runtime/fiber_task.cpp
    src/runtime/script_coroutines.cpp
    src/runtime/save_trace.cpp
    src/runtime/transition_trace.cpp
)

if(WIN32)
    add_executable(cod3pc WIN32 ${COD3PC_SOURCES})
else()
    add_executable(cod3pc ${COD3PC_SOURCES})
endif()

rexglue_setup_target(cod3pc GPU_PLUGINS xenos)
if(WIN32)
    target_compile_definitions(cod3pc PRIVATE $<$<CONFIG:RelWithDebInfo>:COD3_GRASS_GPU_AVAILABLE=1>)
endif()
if(TARGET cod3pc_saint_lo)
    target_sources(cod3pc_saint_lo PRIVATE src/runtime/level_trace.cpp)
endif()
if(WIN32)
    target_link_libraries(cod3pc PRIVATE dbghelp)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(cod3_jump_test ../tests/guest_jump_test.cpp)
    target_include_directories(cod3_jump_test PRIVATE generated/default)
    target_link_libraries(cod3_jump_test PRIVATE rex::runtime)
    rexglue_apply_target_settings(cod3_jump_test)
    add_test(NAME guest_nonlocal_jump COMMAND cod3_jump_test)
    add_executable(cod3_fiber_test ../tests/fiber_task_test.cpp src/runtime/fiber_task.cpp)
    rexglue_apply_target_settings(cod3_fiber_test)
    add_test(NAME script_fiber_continuations COMMAND cod3_fiber_test)
endif()
