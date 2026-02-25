cmake_minimum_required(VERSION 3.15)
project(PlexomeCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Thread support
find_package(Threads REQUIRED)

# 2. Include directory
include_directories(include)

# 3. Source File Registry
set(SOURCES
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
