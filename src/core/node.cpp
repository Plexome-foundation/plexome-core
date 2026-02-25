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
    std::cout << "[System] Node ID: " << config_.node_id << " Starting..." << std::endl;
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
    is_running_ = true;

    if (config_.is_seed) {
        conn_manager_->start_server(static_cast<int>(config_.port));
    } else {
        std::cout << "[Network] Connecting to " << config_.seed_host << "..." << std::endl;
        for(int i = 0; i < 5; ++i) {
            if (conn_manager_->connect_to_seed(config_.seed_host, static_cast<int>(config_.port))) break;
            std::cout << "[Network] Retrying... (" << i+1 << "/5)" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    std::thread cli_thread(&Node::run_cli, this);
    cli_thread.detach(); 

    while (is_running_) {
        knowledge_->scan_new_data();
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
                      << "  ask <text> : Send prompt to AI\n"
                      << "  stats      : Node status\n"
                      << "  peers      : Active connections\n"
                      << "  ls         : Model info\n"
                      << "  exit       : Shutdown\n" << std::endl;
        }
        else if (input == "stats") {
            std::cout << "\n--- Stats ---\n"
                      << "  ID:    " << config_.node_id << "\n"
                      << "  Port:  " << config_.port << "\n"
                      << "  Role:  " << (config_.is_seed ? "SEED" : "PEER") << "\n" << std::endl;
        }
        else if (input == "peers") {
            std::cout << "[Network] Peers: " << conn_manager_->get_active_peers_count() << std::endl;
        }
        else if (input == "ls") {
            std::cout << "[AI Core] Model loaded and ready." << std::endl;
        }
        else if (input.rfind("ask ", 0) == 0) {
            std::cout << "\n[Response]: " << engine_->predict(input.substr(4)) << "\n" << std::endl;
        }
        else {
            std::cout << "Unknown command. Type 'help'." << std::endl;
        }
    }
}

void Node::stop() {
    is_running_ = false;
    if (conn_manager_) conn_manager_->stop();
}

} // namespace plexome
