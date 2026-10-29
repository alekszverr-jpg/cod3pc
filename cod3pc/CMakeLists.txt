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
    src/runtime/gpu_query_trace.cpp
    src/runtime/movie_skip.cpp
    src/runtime/pc_menu.cpp
    src/runtime/pc_input.cpp
    src/runtime/native_video_menu.cpp
    src/runtime/menu_prompts.cpp
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
foreach(COD3_LEVEL IN ITEMS saint_lo island nightd)
    if(TARGET cod3pc_${COD3_LEVEL})
        target_sources(cod3pc_${COD3_LEVEL} PRIVATE src/runtime/level_yield.cpp)
    endif()
endforeach()
if(TARGET cod3pc_island)
    target_sources(cod3pc_island PRIVATE src/runtime/island_entry_trace.cpp)
endif()
if(TARGET cod3pc_nightd)
    target_sources(cod3pc_nightd PRIVATE src/runtime/nightd_entry_trace.cpp)
endif()
if(WIN32)
    target_link_libraries(cod3pc PRIVATE dbghelp shell32)
    add_executable(cod3_dump_threads src/runtime/dump_title_threads.cpp)
    target_link_libraries(cod3_dump_threads PRIVATE dbghelp)
    add_dependencies(cod3pc cod3_dump_threads)
endif()

include(CTest)
if(BUILD_TESTING)
    # Explicit lifecycle diagnostic, separate from the user's game executable.
    add_executable(cod3_runtime_ownership_probe EXCLUDE_FROM_ALL ../tests/runtime_ownership_probe.cpp src/runtime/fiber_task.cpp)
    # Bind the actual pinned SDL driver and event injection to one SDL instance.
    # rexruntime and consumers otherwise carry separate static SDL copies.
    target_sources(cod3_runtime_ownership_probe PRIVATE ../external/rexglue-sdk/src/input/sdl/sdl_input_driver.cpp)
    target_link_libraries(cod3_runtime_ownership_probe PRIVATE rex::runtime)
    rexglue_apply_target_settings(cod3_runtime_ownership_probe)
    set_target_properties(cod3_runtime_ownership_probe PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ownership-probe")
    add_custom_command(TARGET cod3_runtime_ownership_probe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:rex::runtime>
            $<TARGET_FILE_DIR:cod3_runtime_ownership_probe>)
    add_test(NAME native_runtime_ownership COMMAND cod3_runtime_ownership_probe)
    set_tests_properties(native_runtime_ownership PROPERTIES TIMEOUT 30)
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
    add_executable(cod3_menu_picture_test ../tests/menu_picture_test.cpp)
    rexglue_apply_target_settings(cod3_menu_picture_test)
    add_test(NAME native_menu_picture COMMAND cod3_menu_picture_test)
    add_executable(cod3_video_options_test ../tests/video_options_test.cpp)
    rexglue_apply_target_settings(cod3_video_options_test)
    add_test(NAME native_video_options COMMAND cod3_video_options_test)
    add_executable(cod3_menu_number_test ../tests/menu_number_test.cpp)
    rexglue_apply_target_settings(cod3_menu_number_test)
    add_test(NAME native_menu_number COMMAND cod3_menu_number_test)
    add_executable(cod3_pc_input_test ../tests/pc_input_test.cpp)
    target_link_libraries(cod3_pc_input_test PRIVATE rex::runtime)
    rexglue_apply_target_settings(cod3_pc_input_test)
    add_test(NAME native_pc_input COMMAND cod3_pc_input_test)
    add_executable(cod3_raw_mouse_driver_test ../tests/raw_mouse_driver_test.cpp)
    target_link_libraries(cod3_raw_mouse_driver_test PRIVATE rex::runtime)
    rexglue_apply_target_settings(cod3_raw_mouse_driver_test)
    add_test(NAME native_raw_mouse_driver COMMAND cod3_raw_mouse_driver_test)
    add_executable(cod3_xma_output_valid_test ../tests/xma_output_valid_test.cpp)
    target_link_libraries(cod3_xma_output_valid_test PRIVATE rex::runtime)
    rexglue_apply_target_settings(cod3_xma_output_valid_test)
    add_test(NAME native_xma_output_valid COMMAND cod3_xma_output_valid_test)
    set_tests_properties(native_xma_output_valid PROPERTIES TIMEOUT 15)
    add_executable(cod3_config_roundtrip_test ../tests/config_roundtrip_test.cpp)
    target_link_libraries(cod3_config_roundtrip_test PRIVATE rex::runtime)
    rexglue_apply_target_settings(cod3_config_roundtrip_test)
    add_test(NAME native_config_roundtrip COMMAND cod3_config_roundtrip_test)
    rexglue_apply_target_settings(cod3_thread_exit_test)
    add_test(NAME native_thread_exit_probe COMMAND cod3_thread_exit_test)
    set_tests_properties(native_thread_exit_probe PROPERTIES TIMEOUT 10)
endif()
