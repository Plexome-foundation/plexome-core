#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <vector>

// Предварительное объявление классов (Forward Declaration), 
// чтобы не раздувать зависимости в заголовочном файле
namespace plexome {
    class ConnectionManager;
    class KnowledgeManager;

    // Структура конфигурации ноды
    struct AppConfig {
        std::string node_id = "PXM-NODE-01";
        int port = 7539;
        bool is_seed = false;               // true для Windows Server 2025
        std::string seed_host = "seed1.plexome.ai"; // DNS адрес
        std::string storage_path = "./storage";
    };

    class Node {
    public:
        explicit Node(const AppConfig& config);
        ~Node();

        // Инициализация ресурсов
        void init();

        // Запуск основного цикла ноды (блокирующий вызов)
        void run();

        // Остановка ноды
        void stop();

    private:
        // Поток для работы консоли (CLI)
        void run_cli();

        // Внутренняя логика обработки данных (Gossip, Tasks и т.д.)
        void process_core_logic();

        AppConfig config_;
        std::atomic<bool> is_running_;

        // Умные указатели на подсистемы
        std::unique_ptr<ConnectionManager> conn_manager_;
        std::unique_ptr<KnowledgeManager> knowledge_;
    };
}
