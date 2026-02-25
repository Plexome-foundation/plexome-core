cmake_minimum_required(VERSION 3.20)
project(PlexomeCore VERSION 2.0.0 LANGUAGES C CXX)

# Standard C++ version for the whole project
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define output directories for binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 1. External Dependency: llama.cpp
# We treat it as a subdirectory. It will produce its own libraries.
if (MSVC)
    add_compile_options(/Zc:char8_t-)
endif()
add_subdirectory(llama.cpp)

# 2. Global Includes
include_directories(include)
include_directories(llama.cpp/include)
include_directories(llama.cpp/common)

# 3. Plexome Modules (Target DLLs)
# Note: We will add source files for these as we progress
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

# 4. Plexome Host (The Main Executable)
add_executable(plexome_host src/host/main.cpp)

# Link host with platform-specific dynamic loading libraries
if(WIN32)
    # No extra libs needed for LoadLibrary on Windows
else()
    target_link_libraries(plexome_host PRIVATE dl)
endif()

# Summary message
message(STATUS "Plexome 2.0 configured for ${CMAKE_SYSTEM_NAME}")
