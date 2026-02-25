/**
 * PLEXOME FOUNDATION | Universal Swarm Node v1.3 (Enterprise Tier)
 * Platform: Windows (Primary)
 */

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

// Твои кастомные заголовки (убедись, что они созданы или закомментируй лишние)
#include "connection_manager.h"
#include "knowledge_manager.h"
// #include "inference_engine.h" // Раскомментируй, когда добавим llama.cpp

#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <filesystem>
#include <csignal>
#include <mutex>
#include <condition_variable>
#include <vector>

// Глобальное управление завершением
std::atomic<bool> g_is_running{true};
std::condition_variable g_shutdown_cv;
std::mutex g_shutdown_mtx;

void signal_handler(int signal) {
    if (g_is_running) {
        std::cout << "\n[System] Signal (" << signal << ") received. Shutting down..." << std::endl;
        g_is_running = false;
        g_shutdown_cv.notify_all();
    }
}

namespace plexome {
    // Временная структура конфига, если config_loader еще не готов
    struct AppConfig {
        std::string node_id = "PXM-NODE-01";
        int port = 7539;
        bool is_seed = false;
        std::string seed_host = "seed1.plexome.ai";
        std::string storage_path = "./storage";
    };
}

class PlexomeNode {
public:
    PlexomeNode() {
        // 1. Загрузка конфига (здесь можно вызвать твой ConfigLoader)
        // config_ = plexome::ConfigLoader::load_file("plexome.conf");

        // 2. Инициализация сетевого и контентного движков
        conn_manager_ = std::make_unique<plexome::ConnectionManager>();
        knowledge_ = std::make_unique<plexome::KnowledgeManager>();
        
        std::cout << "[System] Infrastructure initialized. Role: " 
                  << (config_.is_seed ? "SEED" : "PEER") << std::endl;
    }

    void start() {
        std::cout << "=========================================" << std::endl;
        std::cout << "    PLEXOME FOUNDATION | NODE TERMINAL    " << std::endl;
        std::cout << "=========================================" << std::endl;

        // A. Запуск сети
        if (config_.is_seed) {
            conn_manager_->start_server(config_.port);
        } else {
            std::cout << "[Network] Locating Swarm via DNS: " << config_.seed_host << std::endl;
            conn_manager_->connect_to_seed(config_.seed_host, config_.port);
        }

        // B. Запуск потока CLI (чтобы не блокировать основной цикл)
        std::thread cli_thread(&PlexomeNode::run_cli, this);
        cli_thread.detach(); 

        // C. Основной цикл операционной логики
        std::cout << "[Core] Node is operational. Port: " << config_.port << std::endl;
        
        while (g_is_running) {
            process_core_logic();
            
            // Ждем 200мс или сигнала завершения
            std::unique_lock<std::mutex> lock(g_shutdown_mtx);
            if (g_shutdown_cv.wait_for(lock, std::chrono::milliseconds(200), []{ return !g_is_running.load(); })) {
                break;
            }
        }

        perform_shutdown();
    }

private:
    plexome::AppConfig config_;
    std::unique_ptr<plexome::ConnectionManager> conn_manager_;
    std::unique_ptr<plexome::KnowledgeManager> knowledge_;

    void run_cli() {
        std::string input;
        while (g_is_running) {
            std::cout << "pxm> " << std::flush;
            if (!std::getline(std::cin, input)) break;
            if (input.empty()) continue;

            if (input == "exit" || input == "quit") {
                g_is_running = false;
                g_shutdown_cv.notify_all();
                break;
            } else if (input == "peers") {
                std::cout << "[Network] Active connections: " << conn_manager_->get_active_peers_count() << std::endl;
            } else if (input == "help") {
                std::cout << "Commands: peers, stats, exit" << std::endl;
            } else {
                std::cout << "Unknown command. Type 'help'." << std::endl;
            }
        }
    }

    void process_core_logic() {
        // Здесь будет вызов Gossip, TaskManager и в будущем - Inference
        // Пока просто проверяем новые файлы
        // knowledge_->scan_knowledge_base("./knowledge");
    }

    void perform_shutdown() {
        std::cout << "[System] Closing sockets and saving state..." << std::endl;
        conn_manager_->stop();
        std::cout << "[System] Plexome Node terminated." << std::endl;
    }
};

int main(int argc, char* argv[]) {
    // Регистрация сигналов для корректного выхода (Ctrl+C)
    std::signal(SIGINT, signal_handler);  
    std::signal(SIGTERM, signal_handler); 

#ifdef _WIN32
    // Инициализация сокетов Windows
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock init failed" << std::endl;
        return 1;
    }
#endif

    try {
        PlexomeNode node;
        node.start();
    } catch (const std::exception& e) { 
        std::cerr << "[Fatal Error] " << e.what() << std::endl; 
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
