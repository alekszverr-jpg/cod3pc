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
)

if(WIN32)
    add_executable(cod3pc WIN32 ${COD3PC_SOURCES})
else()
    add_executable(cod3pc ${COD3PC_SOURCES})
endif()

rexglue_setup_target(cod3pc GPU_PLUGINS xenos)
