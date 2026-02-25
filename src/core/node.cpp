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
    : config_(config), 
      is_running_(false) {
    
    conn_manager_ = std::make_unique<ConnectionManager>();
    knowledge_ = std::make_unique<KnowledgeManager>(config_.storage_path);
    engine_ = std::make_unique<InferenceEngine>(); 
}

Node::~Node() {
    stop();
}

void Node::init() {
    std::cout << "[System] Initializing Plexome Enterprise Node..." << std::endl;
    std::cout << "[System] Node ID: " << config_.node_id << " | Port: " << config_.port << std::endl;

    if (!std::filesystem::exists(config_.storage_path)) {
        std::filesystem::create_directories(config_.storage_path);
    }
    
    // Создаем папку для моделей, если её нет
    if (!std::filesystem::exists("./models")) {
        std::filesystem::create_directories("./models");
    }

    // Автоматический поиск и загрузка первой GGUF модели
    std::cout << "[AI Core] Scanning ./models for GGUF files..." << std::endl;
    for (const auto& entry : std::filesystem::directory_iterator("./models")) {
        if (entry.path().extension() == ".gguf") {
            std::cout << "[AI Core] Loading: " << entry.path().filename() << std::endl;
            if (engine_->load_model(entry.path().string())) {
                break; 
            }
        }
    }
}

void Node::run() {
    is_running_ = true;

    if (config_.is_seed) {
        conn_manager_->start_server(static_cast<int>(config_.port));
    } else {
        std::cout << "[Network] Connecting to seed: " << config_.seed_host << std::endl;
        conn_manager_->connect_to_seed(config_.seed_host, static_cast<int>(config_.port));
    }

    std::thread cli_thread(&Node::run_cli, this);
    cli_thread.detach(); 

    while (is_running_) {
        process_core_logic();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
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
        else if (input == "ls") {
            std::cout << "[AI Core] Model status: " << engine_->get_loaded_model_name() << std::endl;
        }
        else if (input.rfind("ask ", 0) == 0) {
            std::string prompt = input.substr(4);
            std::cout << "[AI Core] Processing..." << std::endl;
            std::string res = engine_->predict(prompt);
            std::cout << "\n[AI]: " << res << "\n" << std::endl;
        }
        else if (input == "peers") {
            std::cout << "[Network] Active peers: " << conn_manager_->get_active_peers_count() << std::endl;
        }
        else if (input == "help") {
            std::cout << "\nCommands: ask <text>, ls, peers, stats, exit\n" << std::endl;
        }
    }
}

void Node::process_core_logic() {
    // Сканирование новых документов для базы знаний
    auto new_docs = knowledge_->scan_new_data();
    if (!new_docs.empty()) {
        std::cout << "\n[Knowledge] New data detected: " << new_docs.size() << " items." << std::endl;
        std::cout << "pxm> " << std::flush;
    }
}

void Node::stop() {
    is_running_ = false;
    if (conn_manager_) conn_manager_->stop();
}

} // namespace plexome
