cmake_minimum_required(VERSION 3.15)
project(PlexomeCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define directories
include_directories(include)

# Source files
set(SOURCES
    src/core/node.cpp
    src/core/memory_manager.cpp
)

# Main executable
add_executable(plexome_node ${SOURCES})

# Target properties
set_target_properties(plexome_node PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

message(STATUS "Plexome Architecture: Prepared for build")
