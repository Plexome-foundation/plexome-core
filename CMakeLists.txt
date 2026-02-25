cmake_minimum_required(VERSION 3.15)
project(PlexomeCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include_directories(include)

set(SOURCES
    src/core/node.cpp
    src/core/memory_manager.cpp
    src/core/shard_storage.cpp
    src/core/task_manager.cpp
    src/core/consensus_engine.cpp
    src/core/integrity_checker.cpp
    src/core/stats_collector.cpp
    src/core/config_loader.cpp
    src/network/bootstrap_manager.cpp
    src/network/connection_manager.cpp
    src/network/gossip_service.cpp
    src/core/inference_engine.cpp
)

add_executable(plexome_node ${SOURCES})
target_link_libraries(plexome_node PUBLIC Threads::Threads stdc++fs)

message(STATUS "Plexome Full Build Configuration: COMPLETE")
