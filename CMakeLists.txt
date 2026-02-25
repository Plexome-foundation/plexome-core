cmake_minimum_required(VERSION 3.15)
project(PlexomeCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Add all our sources
set(SOURCES
    src/core/node.cpp
    src/core/memory_manager.cpp
    src/core/shard_storage.cpp
)

add_executable(plexome_node ${SOURCES})

# Links for filesystem support on older compilers if needed
target_link_libraries(plexome_node PUBLIC stdc++fs)

message(STATUS "Plexome Core Build System Ready")
