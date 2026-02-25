#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <vector>
#include "plexome_types.h" // Подключаем твои общие типы

namespace plexome {
    // Предварительное объявление классов
    class ConnectionManager;
    class KnowledgeManager;

    // УДАЛИЛИ отсюда struct AppConfig, так как она уже есть в plexome_types.h

    class Node {
    public:
        // Используем тип из plexome_types.h
        explicit Node(const AppConfig& config);
        ~Node();

        void init();
        void run();
        void stop();

    private:
        void run_cli();
        void process_core_logic();

        AppConfig config_;
        std::atomic<bool> is_running_;

        std::unique_ptr<ConnectionManager> conn_manager_;
        std::unique_ptr<KnowledgeManager> knowledge_;
    };
}
