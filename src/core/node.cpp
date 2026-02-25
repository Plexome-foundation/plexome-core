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
    engine_ = InferenceEngine::create(); 
}

Node::~Node() {
    stop();
}

void Node::init() {
    std::cout << "[System] Node ID: " << config_.node_id << " Starting..." << std::endl;

    if (!std::filesystem::exists(config_.storage_path)) {
        std::filesystem::create_directories(config_.storage_path);
    }
    
    if (!std::filesystem::exists("./models")) {
        std::filesystem::create_directories("./models");
    }

    // Загрузка модели GGUF
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
    is_running_ = true;

    if (config_.is_seed) {
        conn_manager_->start_server(static_cast<int>(config_.port));
    } else {
        conn_manager_->connect_to_seed(config_.seed_host, static_cast<int>(config_.port));
    }

    std::thread cli_thread(&Node::run_cli, this);
    cli_thread.detach(); 

    while (is_running_) {
        process_core_logic();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
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
            std::cout << "\n--- Available Commands ---\n"
                      << "  ask <prompt> : Send text to the AI model\n"
                      << "  stats        : Show node identity and resources\n"
                      << "  peers        : List active swarm connections\n"
                      << "  ls           : Show current model info\n"
                      << "  help         : Show this menu\n"
                      << "  exit         : Shutdown node\n" << std::endl;
        }
        else if (input == "stats") {
            std::cout << "\n--- Plexome Node Stats ---\n"
                      << "  Node ID:   " << config_.node_id << "\n"
                      << "  Role:      " << (config_.is_seed ? "SEED" : "PEER") << "\n"
                      << "  Port:      " << config_.port << "\n"
                      << "  Storage:   " << config_.storage_path << "\n"
                      << "  Status:    ONLINE\n" << std::endl;
        }
        else if (input.rfind("ask ", 0) == 0) {
            std::string prompt = input.substr(4);
            std::cout << "[AI Core] Thinking..." << std::endl;
            std::string response = engine_->predict(prompt);
            std::cout << "\n[Response]: " << response << "\n" << std::endl;
        }
        else if (input == "peers") {
            size_t count = conn_manager_->get_active_peers_count();
            std::cout << "[Network] Connected peers: " << count << std::endl;
        }
        else if (input == "ls") {
             std::cout << "[AI Core] Engine initialized and model loaded." << std::endl;
        }
        else {
            std::cout << "Unknown command. Type 'help' for a list of commands." << std::endl;
        }
    }
}

void Node::process_core_logic() {
    knowledge_->scan_new_data();
}

void Node::stop() {
    is_running_ = false;
    if (conn_manager_) conn_manager_->stop();
}

} // namespace plexome
