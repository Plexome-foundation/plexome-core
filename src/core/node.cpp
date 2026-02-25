#include "node.h"
#include "connection_manager.h"
#include "knowledge_manager.h"
#include "inference_engine.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>

namespace plexome {

Node::Node(const AppConfig& config) 
    : config_(config), is_running_(false) {
    conn_manager_ = std::make_unique<ConnectionManager>();
    knowledge_ = std::make_unique<KnowledgeManager>(config_.storage_path);
    
    // ВАЖНО: Вызываем фабричный метод, а не абстрактный класс
    engine_ = InferenceEngine::create(); 
}

Node::~Node() { stop(); }

void Node::init() {
    std::cout << "[System] Node ID: " << config_.node_id << " Starting..." << std::endl;

    if (!std::filesystem::exists("./models")) std::filesystem::create_directories("./models");

    // Считывание модели
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
    if (config_.is_seed) conn_manager_->start_server(config_.port);
    else conn_manager_->connect_to_seed(config_.seed_host, config_.port);

    std::thread cli(&Node::run_cli, this);
    cli.detach();

    while (is_running_) {
        process_core_logic();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void Node::run_cli() {
    std::string cmd;
    while (is_running_) {
        std::cout << "pxm> " << std::flush;
        if (!std::getline(std::cin, cmd) || cmd == "exit") { is_running_ = false; break; }
        if (cmd.rfind("ask ", 0) == 0) {
            std::cout << engine_->predict(cmd.substr(4)) << std::endl;
        }
    }
}

void Node::process_core_logic() {
    knowledge_->scan_new_data();
}

void Node::stop() { is_running_ = false; }

} // namespace plexome
