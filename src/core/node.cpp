#include "node.h"
#include "connection_manager.h"
#include "knowledge_manager.h"
#include "inference_engine.h" // Убедись, что этот заголовок есть
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
    
    // Инициализируем движок ИИ
    // Предполагаем, что Factory создает дефолтный инстанс (llama.cpp и т.д.)
    engine_ = std::make_unique<InferenceEngine>(); 
}

Node::~Node() {
    stop();
}

void Node::init() {
    std::cout << "[System] Initializing Plexome Enterprise Node..." << std::endl;
    std::cout << "[System] Node ID: " << config_.node_id << " | Port: " << config_.port << std::endl;

    // 1. Проверка директорий
    if (!std::filesystem::exists(config_.storage_path)) {
        std::filesystem::create_directories(config_.storage_path);
    }
    if (!std::filesystem::exists("./models")) {
        std::filesystem::create_directories("./models");
    }

    // 2. АВТО-ЗАГРУЗКА МОДЕЛИ
    std::cout << "[AI Core] Searching for models in ./models..." << std::endl;
    bool model_found = false;
    for (const auto& entry : std::filesystem::directory_iterator("./models")) {
        if (entry.path().extension() == ".gguf") {
            std::cout << "[AI Core] Loading model: " << entry.path().filename() << std::endl;
            if (engine_->load_model(entry.path().string())) {
                model_found = true;
                break; 
            }
        }
    }

    if (!model_found) {
        std::cout << "[AI Core] Warning: No .gguf models found in ./models folder." << std::endl;
    }
}

void Node::run() {
    is_running_ = true;

    // Сетевой запуск
    if (config_.is_seed) {
        conn_manager_->start_server(static_cast<int>(config_.port));
    } else {
        std::cout << "[Network] Joining Swarm via DNS: " << config_.seed_host << std::endl;
        conn_manager_->connect_to_seed(config_.seed_host, static_cast<int>(config_.port));
    }

    // Запуск CLI
    std::thread cli_thread(&Node::run_cli, this);
    cli_thread.detach(); 

    // ГЛАВНЫЙ ЦИКЛ ОПЕРАЦИЙ
    while (is_running_) {
        process_core_logic();
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Проверка раз в 5 сек
    }

    stop();
}

void Node::process_core_logic() {
    // 1. Считывание новых мануалов из папки ./knowledge
    auto new_files = knowledge_->scan_new_data();
    if (!new_files.empty()) {
        std::cout << "\n[Knowledge] Ingested " << new_files.size() << " new documents." << std::endl;
        std::cout << "pxm> " << std::flush;
        // Здесь логика передачи в TrainingController для создания LoRA-задач
    }

    // 2. Проверка очереди задач из сети (Swarm Tasks)
    // auto task = task_manager_->pull_next_task(); 
    // if (task) engine_->execute(task);
}

void Node::run_cli() {
    std::string input;
    while (is_running_) {
        std::cout << "pxm> " << std::flush;
        if (!std::getline(std::cin, input)) break;
        if (input.empty()) continue;

        if (input == "exit") {
            is_running_ = false;
            break;
        } 
        else if (input == "ls") {
            std::cout << "[AI Core] Current model: " << engine_->get_loaded_model_name() << std::endl;
        }
        else if (input.rfind("ask ", 0) == 0) {
            std::string prompt = input.substr(4);
            std::cout << "[AI Core] Thinking...\n";
            std::string response = engine_->predict(prompt);
            std::cout << "\n[Response]: " << response << "\n";
        }
        else if (input == "peers") {
            std::cout << "[Network] Connections: " << conn_manager_->get_active_peers_count() << std::endl;
        }
        else {
            std::cout << "Unknown command. Use: peers, ls, ask <prompt>, exit" << std::endl;
        }
    }
}

void Node::stop() {
    is_running_ = false;
    conn_manager_->stop();
}

} // namespace plexome
