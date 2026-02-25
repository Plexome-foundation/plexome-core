#include "node.h"
#include "connection_manager.h"
#include "knowledge_manager.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>

namespace plexome {

Node::Node(const AppConfig& config) 
    : config_(config), 
      is_running_(false) {
    
    // Инициализация Enterprise-компонентов
    conn_manager_ = std::make_unique<ConnectionManager>();
    knowledge_ = std::make_unique<KnowledgeManager>();
    
    // Здесь в будущем инициализируются остальные модули:
    // stats_ = std::make_unique<StatsCollector>();
    // task_manager_ = std::make_unique<TaskManager>();
}

Node::~Node() {
    stop();
}

void Node::init() {
    std::cout << "[System] Initializing Plexome Enterprise Node..." << std::endl;
    std::cout << "[System] Node ID: " << config_.node_id << " | Port: " << config_.port << std::endl;

    // Проверка папок
    if (!std::filesystem::exists("./knowledge")) {
        std::filesystem::create_directory("./knowledge");
    }
}

void Node::run() {
    is_running_ = true;

    // 1. СЕТЕВОЙ ЗАПУСК (DNS / SEED)
    if (config_.is_seed) {
        // Логика для Windows Server 2025
        if (!conn_manager_->start_server(config_.port)) {
            std::cerr << "[Network] Critical: Could not bind port " << config_.port << std::endl;
            return;
        }
    } else {
        // Логика для Windows 11 (Клиенты)
        std::cout << "[Network] Joining Swarm via DNS: " << config_.seed_host << std::endl;
        if (!conn_manager_->connect_to_seed(config_.seed_host, config_.port)) {
            std::cout << "[Network] Seed unreachable. Standing by for incoming peers..." << std::endl;
        }
    }

    // 2. ЗАПУСК CLI В ОТДЕЛЬНОМ ПОТОКЕ
    // Это важно, чтобы основной цикл process_core_logic не зависел от ввода пользователя
    std::thread cli_thread(&Node::run_cli, this);
    cli_thread.detach(); 

    // 3. ОСНОВНОЙ ОПЕРАЦИОННЫЙ ЦИКЛ (Event Loop)
    std::cout << "[Core] Entering main operational loop." << std::endl;
    
    while (is_running_) {
        process_core_logic();
        
        // Небольшая пауза, чтобы не выжирать 100% CPU в пустом цикле
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    stop();
}

void Node::run_cli() {
    std::string input;
    while (is_running_) {
        std::cout << "pxm> " << std::flush;
        
        if (!std::getline(std::cin, input)) break;
        if (input.empty()) continue;

        if (input == "exit" || input == "quit") {
            std::cout << "[Core] Exit command received." << std::endl;
            is_running_ = false;
            break;
        } 
        else if (input == "peers") {
            size_t count = conn_manager_->get_active_peers_count();
            std::cout << "[Network] Connected peers: " << count << std::endl;
        } 
        else if (input == "stats") {
            std::cout << "\n--- Node Stats ---\n"
                      << "Role: " << (config_.is_seed ? "SEED (Server)" : "PEER (Client)") << "\n"
                      << "Host: " << (config_.is_seed ? "localhost" : config_.seed_host) << "\n"
                      << "ID:   " << config_.node_id << "\n\n";
        }
        else if (input == "help") {
            std::cout << "\nCommands: peers, stats, exit, help\n" << std::endl;
        }
        else {
            std::cout << "Unknown command. Type 'help'." << std::endl;
        }
    }
}

void Node::process_core_logic() {
    // 1. Сканирование новых мануалов
    // knowledge_->scan_new_data();
    
    // 2. Здесь будет Gossip протокол и обмен задачами
}

void Node::stop() {
    if (is_running_) {
        is_running_ = false;
    }
    if (conn_manager_) {
        conn_manager_->stop();
    }
    std::cout << "[System] Graceful shutdown complete." << std::endl;
}

} // namespace plexome
