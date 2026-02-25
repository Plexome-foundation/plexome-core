cmake_minimum_required(VERSION 3.15)
project(PlexomeCore LANGUAGES CXX)
# ==========================================
# Plexome: Static C++ Runtime Configuration
# ==========================================
if(MSVC)
    # Принудительно используем статическую линковку (/MT вместо /MD)
    # Это сделает .exe независимым от Visual C++ Redistributable
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Thread support
find_package(Threads REQUIRED)

# 2. Include directory
include_directories(include)

set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable llama tests") # чтобы не собирать лишний мусор
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable llama examples")
add_subdirectory(llama.cpp)

# 3. Source File Registry
set(SOURCES
    src/core/main.cpp
    src/core/node.cpp
    src/core/memory_manager.cpp
    src/core/shard_storage.cpp
    src/core/task_manager.cpp
    src/core/consensus_engine.cpp
    src/core/integrity_checker.cpp
    src/core/stats_collector.cpp
    src/core/config_loader.cpp
    src/core/inference_engine.cpp
    src/core/knowledge_manager.cpp
    src/core/training_controller.cpp
    src/core/benchmark.cpp
    src/network/bootstrap_manager.cpp
    src/network/connection_manager.cpp
    src/network/gossip_service.cpp
    src/network/lora_registry.cpp   # NEW: Mechanism A
    src/network/swarm_router.cpp    # NEW: Mechanism B
)

# 4. Create Executable
add_executable(plexome_node ${SOURCES})

# 5. OS-Specific Linking
if(WIN32)
    target_link_libraries(plexome_node PUBLIC Threads::Threads ws2_32)
else()
    target_link_libraries(plexome_node PUBLIC Threads::Threads stdc++fs)
endif()

message(STATUS "Plexome Build System configured for " ${CMAKE_SYSTEM_NAME})
