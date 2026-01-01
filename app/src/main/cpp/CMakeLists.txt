cmake_minimum_required(VERSION 3.10.0...3.31.5)

project("lakesnes")

# Find SDL2
find_library(log-lib log)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(OpenSLES-lib OpenSLES)
# Find SDL2 library
# SDL2 is not included by default in Android NDK
find_library(SDL2-lib 
    NAMES SDL2
    HINTS 
        ${ANDROID_NDK_HOME}/sources/android/SDL2
        ${ANDROID_NDK_HOME}/sources/SDL2
        ${ANDROID_NDK_HOME}/third_party/SDL2
        ${ANDROID_NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib
        /usr/lib
        /usr/local/lib
    NO_DEFAULT_PATH
)

# If not found, try default paths
if(NOT SDL2-lib)
    find_library(SDL2-lib SDL2)
endif()

if(SDL2-lib)
    message(STATUS "SDL2 library found: ${SDL2-lib}")
else()
    message(WARNING "SDL2 library not found")
endif()

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

# Find SDL2 in Android NDK
# SDL2 is not included by default in Android NDK, try multiple locations
find_path(SDL2_INCLUDE_DIR SDL.h
    PATHS 
        ${ANDROID_NDK_HOME}/sources/android/SDL2
        ${ANDROID_NDK_HOME}/sources/SDL2
        ${ANDROID_NDK_HOME}/third_party/SDL2
        ${ANDROID_NDK_HOME}/sources/cxx-stl/llvm-libc++/SDL2
    NO_DEFAULT_PATH
)

# If not found in NDK, try system paths
if(NOT SDL2_INCLUDE_DIR)
    find_path(SDL2_INCLUDE_DIR SDL.h
        HINTS /usr/include /usr/local/include
    )
endif()

# Add SDL2 include directories
if(SDL2_INCLUDE_DIR)
    include_directories(${SDL2_INCLUDE_DIR})
    message(STATUS "SDL2 include directory found: ${SDL2_INCLUDE_DIR}")
else()
    message(WARNING "SDL2 include directory not found")
endif()

# Target link libraries
target_link_libraries(lakesnes
    ${log-lib}
    ${android-lib}
    ${EGL-lib}
    ${OpenSLES-lib}
)

# Link SDL2 if found
if(SDL2-lib)
    target_link_libraries(lakesnes ${SDL2-lib})
    target_compile_definitions(lakesnes PRIVATE HAVE_SDL2=1)
else()
    target_compile_definitions(lakesnes PRIVATE HAVE_SDL2=0)
    message(WARNING "SDL2 not found - emulator will use minimal functionality")
endif()

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