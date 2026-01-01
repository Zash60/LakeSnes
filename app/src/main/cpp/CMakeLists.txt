cmake_minimum_required(VERSION 3.10.0...3.31.5)

project("lakesnes")

# Find SDL2
find_library(log-lib log)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(OpenSLES-lib OpenSLES)
# SDL2 is required for the emulator
# Download and include SDL2 in your project for full functionality
find_library(SDL2-lib SDL2 REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/../../..)
include_directories(${CMAKE_SOURCE_DIR}/../../../../snes)
include_directories(${CMAKE_SOURCE_DIR}/../../../../zip)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the native library
add_library(lakesnes SHARED
    emulator_jni.cpp
    ../../../../snes/spc.c
    ../../../../snes/dsp.c
    ../../../../snes/apu.c
    ../../../../snes/cpu.c
    ../../../../snes/dma.c
    ../../../../snes/ppu.c
    ../../../../snes/cart.c
    ../../../../snes/cx4.c
    ../../../../snes/input.c
    ../../../../snes/statehandler.c
    ../../../../snes/snes.c
    ../../../../snes/snes_other.c
    ../../../../zip/zip.c
    ../../../../tracing.c
)

# SDL2 include directory is required
find_path(SDL2_INCLUDE_DIR SDL.h REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

# Target link libraries
target_link_libraries(lakesnes
    ${SDL2-lib}
    ${log-lib}
    ${android-lib}
    ${EGL-lib}
    ${OpenSLES-lib}
)

# Set compiler flags for optimization and debugging
target_compile_options(lakesnes PRIVATE
    -Wall
    -Wextra
    -O3
    -ffast-math
    -funroll-loops
    -ftree-vectorize
    -march=armv8-a
)

# Debug build flags
target_compile_definitions(lakesnes PRIVATE DEBUG=$<CONFIG:Debug>)

# Strip debug symbols in release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_options(lakesnes PRIVATE
        -Wl,--gc-sections
        -Wl,--strip-all
    )
endif()