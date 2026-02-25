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
    
    conn_manager_ = std::make_unique<ConnectionManager>();
    knowledge_ = std::make_unique<KnowledgeManager>();
}

Node::~Node() {
    stop();
}

void Node::init() {
    std::cout << "[System] Initializing Plexome Enterprise Node..." << std::endl;
    // Используем твои uint16_t и строковые поля
    std::cout << "[System] Node ID: " << config_.node_id << " | Port: " << config_.port << std::endl;

    if (!std::filesystem::exists(config_.storage_path)) {
        std::filesystem::create_directories(config_.storage_path);
    }
}

void Node::run() {
    is_running_ = true;

    // СЕТЕВАЯ ЛОГИКА
    if (config_.is_seed) {
        // Запуск сервера на порту из конфига
        if (!conn_manager_->start_server(static_cast<int>(config_.port))) {
            std::cerr << "[Network] Critical: Could not bind port " << config_.port << std::endl;
            return;
        }
    } else {
        // Подключение к сиду через DNS
        std::cout << "[Network] Joining Swarm via DNS: " << config_.seed_host << std::endl;
        if (!conn_manager_->connect_to_seed(config_.seed_host, static_cast<int>(config_.port))) {
            std::cout << "[Network] Seed unreachable. Mode: Standalone/Passive." << std::endl;
        }
    }

    // Запуск CLI
    std::thread cli_thread(&Node::run_cli, this);
    cli_thread.detach(); 

    std::cout << "[Core] Node is operational. Type 'help' for commands." << std::endl;
    
    // Основной цикл
    while (is_running_) {
        process_core_logic();
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
            is_running_ = false;
            break;
        } 
        else if (input == "peers") {
            size_t count = conn_manager_->get_active_peers_count();
            std::cout << "[Network] Active TCP connections: " << count << std::endl;
        } 
        else if (input == "stats") {
            std::cout << "\n--- Node Stats ---\n"
                      << "Role:    " << (config_.is_seed ? "SEED" : "PEER") << "\n"
                      << "Port:    " << config_.port << "\n"
                      << "Storage: " << config_.storage_path << "\n"
                      << "RAM Lim: " << config_.ram_limit_bytes << " bytes\n\n";
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
    // Резерв под Gossip и задачи
}

void Node::stop() {
    is_running_ = false;
    if (conn_manager_) {
        conn_manager_->stop();
    }
    std::cout << "[System] Node stopped." << std::endl;
}

} // namespace plexome
