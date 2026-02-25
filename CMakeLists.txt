cmake_minimum_required(VERSION 3.15)
project(PlexomeCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(SOURCES
    src/core/node.cpp
    src/core/memory_manager.cpp
    src/core/shard_storage.cpp
    src/core/task_manager.cpp
    src/core/consensus_engine.cpp
    src/network/bootstrap_manager.cpp
)

add_executable(plexome_node ${SOURCES})
target_link_libraries(plexome_node PUBLIC stdc++fs)

message(STATUS "Plexome Production Build Ready")
