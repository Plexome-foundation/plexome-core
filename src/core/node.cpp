#include "node.h"
#include "connection_manager.h"
#include "knowledge_manager.h"
#include "inference_engine.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>

namespace plexome {

Node::Node(const AppConfig& config) 
    : config_(config), is_running_(false) {
    conn_manager_ = std::make_unique<ConnectionManager>();
    knowledge_ = std::make_unique<KnowledgeManager>(config_.storage_path);
    engine_ = InferenceEngine::create(); 
}

Node::~Node() { stop(); }

void Node::init() {
    std::cout << "[System] Node ID: " << config_.node_id << " Initializing..." << std::endl;
    if (!std::filesystem::exists("./models")) std::filesystem::create_directories("./models");

    for (const auto& entry : std::filesystem::directory_iterator("./models")) {
        if (entry.path().extension() == ".gguf") {
            if (engine_->load_model(entry.path().string())) {
                std::cout << "[AI] Model Loaded: " << entry.path().filename() << std::endl;
                break; 
            }
        }
    }
}

void Node::run() {
    if (config_.is_seed) {
        conn_manager_->start_server(static_cast<int>(config_.port));
    } else {
        std::cout << "[Network] Connecting to " << config_.seed_host << "..." << std::endl;
        for(int i = 0; i < 5; ++i) {
            if (conn_manager_->connect_to_seed(config_.seed_host, static_cast<int>(config_.port))) break;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    is_running_ = true;
    std::thread cli_thread(&Node::run_cli, this);
    cli_thread.detach(); 

    while (is_running_) {
        process_core_logic();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Чаще проверяем сообщения
    }
}

void Node::process_core_logic() {
    // 1. Проверяем входящие сообщения из сети
    std::string msg;
    while (conn_manager_->get_next_message(msg)) {
        if (msg.rfind("ASK:", 0) == 0) {
            std::string prompt = msg.substr(4);
            std::cout << "\n[Swarm Request] Peer asks: " << prompt << std::endl;
            
            if (engine_->is_loaded()) {
                std::cout << "[AI Core] Processing distributed task..." << std::endl;
                std::string answer = engine_->predict(prompt);
                conn_manager_->broadcast("ANS:" + answer);
                std::cout << "[Swarm Request] Answer sent back to peer.\npxm> " << std::flush;
            } else {
                std::cout << "[AI Core] No local model to process the request.\npxm> " << std::flush;
            }
        } 
        else if (msg.rfind("ANS:", 0) == 0) {
            std::string answer = msg.substr(4);
            std::cout << "\n[Swarm AI Response]: " << answer << "\npxm> " << std::flush;
        }
    }

    // 2. Сканируем новые данные
    knowledge_->scan_new_data();
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
        else if (input == "help" || input == "?") {
            std::cout << "\n--- Commands ---\n"
                      << "  ask <text> : Prompt AI (Local or Swarm)\n"
                      << "  stats      : Node info\n"
                      << "  peers      : Connections\n"
                      << "  exit       : Shutdown\n" << std::endl;
        }
        else if (input == "stats") {
            std::cout << "Role: " << (config_.is_seed ? "SEED" : "PEER") 
                      << " | Peers: " << conn_manager_->get_active_peers_count() 
                      << " | Model Loaded: " << (engine_->is_loaded() ? "Yes" : "No") << std::endl;
        }
        else if (input == "peers") {
            std::cout << "[Network] Active: " << conn_manager_->get_active_peers_count() << std::endl;
        }
        else if (input.rfind("ask ", 0) == 0) {
            std::string prompt = input.substr(4);
            if (engine_->is_loaded()) {
                // Если модель есть локально — считаем сами
                std::cout << "\n[Local AI]: " << engine_->predict(prompt) << "\n" << std::endl;
            } else {
                // Если модели нет — отправляем в сеть
                if (conn_manager_->get_active_peers_count() > 0) {
                    std::cout << "[Network] No local model. Broadcasting to Swarm..." << std::endl;
                    conn_manager_->broadcast("ASK:" + prompt);
                } else {
                    std::cout << "[Error] No local model and no peers connected!" << std::endl;
                }
            }
        }
    }
}

void Node::stop() {
    is_running_ = false;
    if (conn_manager_) conn_manager_->stop();
}

} // namespace plexome
