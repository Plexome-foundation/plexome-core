cmake_minimum_required(VERSION 3.15)
# ВАЖНО: Добавлен язык C, так как ядро ggml в llama.cpp написано на чистом C
project(PlexomeCore LANGUAGES CXX C)

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

# 2. Include directory (для твоих файлов)
include_directories(include)

# ==========================================
# Интеграция движка llama.cpp
# ==========================================
# Отключаем сборку тестов, примеров и сервера, чтобы не тратить время компилятора
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable llama tests" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable llama examples" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "Disable llama server" FORCE)

# Подключаем папку (убедись, что папка llama.cpp лежит в корне проекта)
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

# ВАЖНО: Указываем нашему экзешнику, где искать заголовочные файлы llama.cpp
target_include_directories(plexome_node PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp/common
)

# 5. OS-Specific Linking (добавлена линковка с llama)
if(WIN32)
    target_link_libraries(plexome_node PUBLIC Threads::Threads ws2_32 llama)
else()
    target_link_libraries(plexome_node PUBLIC Threads::Threads stdc++fs llama)
endif()

message(STATUS "Plexome Build System configured for " ${CMAKE_SYSTEM_NAME})
