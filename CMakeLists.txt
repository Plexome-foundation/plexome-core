cmake_minimum_required(VERSION 3.20)
project(PlexomeCore VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if (MSVC)
    add_compile_options(/Zc:char8_t-)
endif()

set(GGML_AVX ON CACHE BOOL "Enable AVX" FORCE)
set(GGML_AVX2 ON CACHE BOOL "Enable AVX2" FORCE)
set(GGML_FMA ON CACHE BOOL "Enable FMA" FORCE)
set(GGML_F16C ON CACHE BOOL "Enable F16C" FORCE)
set(GGML_NATIVE ON CACHE BOOL "Enable Native Architecture" FORCE)
# ==============================================================================
# HARDWARE ACCELERATION: OpenBLAS (Universal Intel/AMD Math)
# ==============================================================================
set(GGML_BLAS ON CACHE BOOL "Enable BLAS" FORCE)
set(GGML_BLAS_VENDOR "OpenBLAS" CACHE STRING "BLAS library" FORCE)


set(CMAKE_INCLUDE_PATH "C:/OpenBLAS/include" ${CMAKE_INCLUDE_PATH})
set(CMAKE_LIBRARY_PATH "C:/OpenBLAS/lib" ${CMAKE_LIBRARY_PATH})
set(CMAKE_PREFIX_PATH "C:/OpenBLAS" ${CMAKE_PREFIX_PATH})


set(BLAS_LIBRARIES "C:/OpenBLAS/lib/openblas.lib" CACHE FILEPATH "Path to OpenBLAS library" FORCE)
set(BLAS_INCLUDE_DIRS "C:/OpenBLAS/include" CACHE PATH "Path to OpenBLAS headers" FORCE)
# ==============================================================================
add_subdirectory(llama.cpp)

include_directories(include)
include_directories(llama.cpp/include)
include_directories(llama.cpp/common)

# 3. Plexome Modules
add_library(pxm_ai SHARED modules/ai/pxm_ai.cpp)
target_compile_definitions(pxm_ai PRIVATE PXM_EXPORT)
target_link_libraries(pxm_ai PRIVATE llama)

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

# 4. Plexome Host
add_executable(plexome_host src/host/main.cpp)

if(NOT WIN32)
    target_link_libraries(plexome_host PRIVATE dl)
endif()
