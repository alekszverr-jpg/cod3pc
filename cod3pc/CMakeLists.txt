# cod3pc - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(cod3pc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)
set(COD3_CODEGEN_EXECUTABLE "" CACHE FILEPATH "Pinned codegen with explicit DLL overlay groups")
if(COD3_CODEGEN_EXECUTABLE)
    set_target_properties(rex::rexglue PROPERTIES IMPORTED_LOCATION_RELEASE "${COD3_CODEGEN_EXECUTABLE}")
endif()

# Sources
set(COD3PC_SOURCES
    src/main.cpp
    src/runtime/entry_trace.cpp
    src/runtime/crash_trace.cpp
    src/runtime/fiber_task.cpp
    src/runtime/script_coroutines.cpp
    src/runtime/save_trace.cpp
    src/runtime/transition_trace.cpp
    src/runtime/title_handoff.cpp
    src/runtime/title_relaunch.cpp
    src/runtime/thread_exit_probe.cpp
    src/runtime/title_thread_trace.cpp
    src/runtime/title_gpu_drain.cpp
    src/runtime/worker_exit.cpp
    src/runtime/dof_trace.cpp
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
foreach(COD3_LEVEL IN ITEMS saint_lo island)
    if(TARGET cod3pc_${COD3_LEVEL})
        target_sources(cod3pc_${COD3_LEVEL} PRIVATE src/runtime/level_yield.cpp)
    endif()
endforeach()
if(TARGET cod3pc_island)
    target_sources(cod3pc_island PRIVATE src/runtime/island_entry_trace.cpp)
endif()
if(WIN32)
    target_link_libraries(cod3pc PRIVATE dbghelp shell32)
    add_executable(cod3_dump_threads src/runtime/dump_title_threads.cpp)
    target_link_libraries(cod3_dump_threads PRIVATE dbghelp)
    add_dependencies(cod3pc cod3_dump_threads)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(cod3_thread_gate_test ../tests/title_thread_gate_test.cpp)
    target_link_libraries(cod3_thread_gate_test PRIVATE rex::runtime)
    rexglue_apply_target_settings(cod3_thread_gate_test)
    add_test(NAME native_title_thread_gate COMMAND cod3_thread_gate_test)
    set_tests_properties(native_title_thread_gate PROPERTIES TIMEOUT 10)
    add_executable(cod3_jump_test ../tests/guest_jump_test.cpp)
    target_include_directories(cod3_jump_test PRIVATE generated/default)
    target_link_libraries(cod3_jump_test PRIVATE rex::runtime)
    rexglue_apply_target_settings(cod3_jump_test)
    add_test(NAME guest_nonlocal_jump COMMAND cod3_jump_test)
    add_executable(cod3_fiber_test ../tests/fiber_task_test.cpp src/runtime/fiber_task.cpp)
    rexglue_apply_target_settings(cod3_fiber_test)
    add_test(NAME script_fiber_continuations COMMAND cod3_fiber_test)
    add_executable(cod3_handoff_test ../tests/title_handoff_test.cpp src/runtime/title_handoff.cpp)
    target_link_libraries(cod3_handoff_test PRIVATE shell32)
    rexglue_apply_target_settings(cod3_handoff_test)
    add_test(NAME native_title_handoff COMMAND cod3_handoff_test)
    set_tests_properties(native_title_handoff PROPERTIES TIMEOUT 15)
    add_executable(cod3_thread_exit_test ../tests/thread_exit_probe_test.cpp src/runtime/thread_exit_probe.cpp)
    rexglue_apply_target_settings(cod3_thread_exit_test)
    add_test(NAME native_thread_exit_probe COMMAND cod3_thread_exit_test)
    set_tests_properties(native_thread_exit_probe PROPERTIES TIMEOUT 10)
endif()
