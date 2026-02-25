#include "node.h"
#include "connection_manager.h"
#include "knowledge_manager.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>

namespace plexome {

/**
 * Конструктор: Инициализируем подсистемы.
 * Передаем config_.storage_path в KnowledgeManager, чтобы избежать ошибки C2512.
 */
Node::Node(const AppConfig& config) 
    : config_(config), 
      is_running_(false) {
    
    conn_manager_ = std::make_unique<ConnectionManager>();
    
    // Передаем путь к данным, который требует конструктор KnowledgeManager
    knowledge_ = std::make_unique<KnowledgeManager>(config_.storage_path);
}

Node::~Node() {
    stop();
}

/**
 * Инициализация: Проверка инфраструктуры перед запуском.
 */
void Node::init() {
    std::cout << "[System] Initializing Plexome Enterprise Node..." << std::endl;
    std::cout << "[System] Node ID: " << config_.node_id << " | Port: " << config_.port << std::endl;

    // Создаем директорию для данных, если её нет
    try {
        if (!std::filesystem::exists(config_.storage_path)) {
            std::filesystem::create_directories(config_.storage_path);
            std::cout << "[System] Created storage directory: " << config_.storage_path << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[System] Storage error: " << e.what() << std::endl;
    }
}

/**
 * Основной запуск: Сеть + CLI + Event Loop.
 */
void Node::run() {
    is_running_ = true;

    // 1. Сетевая активация
    if (config_.is_seed) {
        // Режим сервера (Windows Server 2025)
        if (!conn_manager_->start_server(static_cast<int>(config_.port))) {
            std::cerr << "[Network] Critical: Could not bind port " << config_.port << std::endl;
            return;
        }
    } else {
        // Режим клиента (Windows 11) - подключаемся по DNS
        std::cout << "[Network] Joining Swarm via DNS: " << config_.seed_host << std::endl;
        if (!conn_manager_->connect_to_seed(config_.seed_host, static_cast<int>(config_.port))) {
            std::cout << "[Network] Warning: Seed unreachable. Node operating in Standalone mode." << std::endl;
        }
    }

    // 2. Запуск интерфейса управления в отдельном потоке
    std::thread cli_thread(&Node::run_cli, this);
    cli_thread.detach(); 

    std::cout << "[Core] Swarm logic active. Operational path: " << config_.storage_path << std::endl;
    
    // 3. Главный цикл обработки (Gossip, Tasks, Inference)
    while (is_running_) {
        process_core_logic();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    stop();
}

/**
 * Консольный интерфейс ноды.
 */
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
            std::cout << "[Network] Active TCP Connections: " << count << std::endl;
        } 
        else if (input == "stats") {
            std::cout << "\n--- Plexome Node Stats ---\n"
                      << "Node ID:   " << config_.node_id << "\n"
                      << "Role:      " << (config_.is_seed ? "SEED (Root)" : "PEER (Worker)") << "\n"
                      << "Port:      " << config_.port << "\n"
                      << "Storage:   " << config_.storage_path << "\n"
                      << "VRAM Lim:  " << config_.vram_limit_bytes << " bytes\n"
                      << "RAM Lim:   " << config_.ram_limit_bytes << " bytes\n"
                      << "Status:    ONLINE\n\n";
        }
        else if (input == "help") {
            std::cout << "\nAvailable commands:\n"
                      << "  peers  - Show number of connected swarm nodes\n"
                      << "  stats  - Display hardware and identity info\n"
                      << "  help   - Show this menu\n"
                      << "  exit   - Graceful shutdown\n" << std::endl;
        }
        else {
            std::cout << "Unknown command: '" << input << "'. Type 'help'." << std::endl;
        }
    }
}

/**
 * Место для будущей логики распределенных вычислений.
 */
void Node::process_core_logic() {
    // Здесь будет вызываться GossipService и TaskManager
}

/**
 * Остановка всех сервисов.
 */
void Node::stop() {
    if (is_running_) {
        is_running_ = false;
    }
    if (conn_manager_) {
        conn_manager_->stop();
    }
    std::cout << "[System] Node cleanup complete." << std::endl;
}

} // namespace plexome
