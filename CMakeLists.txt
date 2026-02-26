cmake_minimum_required(VERSION 3.20)
project(PlexomeCore VERSION 2.0.0 LANGUAGES C CXX)

# Standard C++ version for the whole project
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================================================================
# Plexome Core: MSVC Runtime Configuration
# Enforce Static MSVC Runtime Linking (/MT) to prevent MSVCP140.dll crashes
# on clean servers. This embeds the C++ standard library into the binaries.
# ==============================================================================
if(MSVC)
    # Modern CMake approach (requires CMake 3.15+)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    # Fallback: Explicitly replace /MD with /MT in default flags
    # (Ensures that nested projects like llama.cpp don't override the setting)
    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()
# ==============================================================================

# Define output directories for binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 1. External Dependency: llama.cpp
# We treat it as a subdirectory. It will produce its own libraries.
if (MSVC)
    add_compile_options(/Zc:char8_t-)
endif()

# ==============================================================================
# Llama.cpp & GGML Strict Compilation Overrides
# ==============================================================================
# 1. Force static C++ runtime (/MT) directly through llama's native variables
set(LLAMA_STATIC_CRTS ON CACHE BOOL "Force static CRT" FORCE)
set(GGML_STATIC_CRTS ON CACHE BOOL "Force static CRT" FORCE)

# 2. Maximum CPU Compatibility Mode (Disable AVX2/FMA for old servers)
set(GGML_AVX2 OFF CACHE BOOL "Disable AVX2" FORCE)
set(GGML_FMA OFF CACHE BOOL "Disable FMA" FORCE)
set(GGML_AVX512 OFF CACHE BOOL "Disable AVX512" FORCE)
# ==============================================================================

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

# Summary message
message(STATUS "Plexome 2.0 configured for ${CMAKE_SYSTEM_NAME} with Static CRT (/MT)")
