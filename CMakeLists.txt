cmake_minimum_required(VERSION 3.20)
project(PlexomeCore VERSION 2.0.0 LANGUAGES C CXX)

# Standard C++ version for the whole project
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================================================================
# Plexome Core: MSVC Runtime Configuration (Static Linking)
# ==============================================================================
if(MSVC)
    # Modern CMake approach to enforce /MT instead of /MD
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/Zc:char8_t-)
endif()

# Define output directories for binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ==============================================================================
# Llama.cpp & GGML Strict Compilation Overrides
# ==============================================================================
# 1. Force static C++ runtime inside llama.cpp
set(LLAMA_STATIC_CRTS ON CACHE BOOL "Force static CRT" FORCE)
set(GGML_STATIC_CRTS ON CACHE BOOL "Force static CRT" FORCE)

# 2. CRITICAL: Disable Native Architecture Auto-Detection
# This prevents MSVC from optimizing for your build machine's modern CPU
set(GGML_NATIVE OFF CACHE BOOL "Disable native architecture" FORCE)
set(LLAMA_NATIVE OFF CACHE BOOL "Disable native architecture" FORCE)

# 3. Maximum CPU Compatibility Mode (Vanilla x86_64)
set(GGML_AVX OFF CACHE BOOL "Disable AVX" FORCE)
set(GGML_AVX2 OFF CACHE BOOL "Disable AVX2" FORCE)
set(GGML_AVX512 OFF CACHE BOOL "Disable AVX512" FORCE)
set(GGML_FMA OFF CACHE BOOL "Disable FMA" FORCE)
set(GGML_F16C OFF CACHE BOOL "Disable F16C" FORCE)
# ==============================================================================

# 1. External Dependency
add_subdirectory(llama.cpp)

# 2. Global Includes
include_directories(include)
include_directories(llama.cpp/include)
include_directories(llama.cpp/common)

# 3. Plexome Modules (Target DLLs)
add_library(pxm_ai SHARED modules/ai/pxm_ai.cpp)
target_compile_definitions(pxm_ai PRIVATE PXM_EXPORT)
target_link_libraries(pxm_ai PRIVATE llama) # Link against llama core

add_library(pxm_network SHARED modules/network/pxm_network.cpp)
target_compile_definitions(pxm_network PRIVATE PXM_EXPORT)

add_library(pxm_topology SHARED modules/topology/pxm_topology.cpp)
target_compile_definitions(pxm_topology PRIVATE PXM_EXPORT)

add_library(pxm_sharder SHARED modules/sharder/pxm_sharder.cpp)
target_compile_definitions(pxm_sharder PRIVATE PXM_EXPORT)

add_library(pxm_updater SHARED modules/updater/pxm_updater.cpp)
target_compile_definitions(pxm_updater PRIVATE PXM_EXPORT)

add_library(pxm_bench SHARED modules/bench/pxm_bench.cpp)
target_compile_definitions(pxm_bench PRIVATE PXM_EXPORT)

add_library(pxm_security SHARED modules/security/pxm_security.cpp)
target_compile_definitions(pxm_security PRIVATE PXM_EXPORT)

add_library(pxm_dashboard SHARED modules/dashboard/pxm_dashboard.cpp)
target_compile_definitions(pxm_dashboard PRIVATE PXM_EXPORT)

# 4. Plexome Host (The Main Executable)
add_executable(plexome_host src/host/main.cpp)

# Link host with platform-specific dynamic loading libraries
if(WIN32)
    # No extra libs needed for LoadLibrary on Windows
else()
    target_link_libraries(plexome_host PRIVATE dl)
endif()

message(STATUS "Plexome 2.0 configured for Universal x86_64 (No AVX, Static CRT)")
