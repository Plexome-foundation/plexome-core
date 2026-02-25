#include "node.h"
#include "plexome_types.h"
#include <iostream>
#include <csignal>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

// Глобальный указатель для корректного завершения по Ctrl+C
plexome::Node* g_node_ptr = nullptr;

void signal_handler(int signal) {
    if (g_node_ptr) {
        std::cout << "\n[System] Shutdown signal received. Cleaning up..." << std::endl;
        g_node_ptr->stop();
    }
}

int main(int argc, char* argv[]) {
    // 1. Инициализация сетевого стека Windows
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[Fatal] Winsock initialization failed." << std::endl;
        return 1;
    }
#endif

    // 2. Настройка конфигурации
    // Позже ты заменишь это на загрузку из файла через config_loader
    plexome::AppConfig config;
    config.node_id = "PXM-MAIN-ALPHA";
    config.port = 7539;
    
    // ВАЖНО: Если запускаешь на сервере 2025, поставь здесь true
    config.is_seed = false; 
    config.seed_host = "seed1.plexome.ai";
    config.storage_path = "./data";

    try {
        // 3. Создание и запуск ноды
        plexome::Node node(config);
        g_node_ptr = &node;

        // Регистрация обработчиков сигналов
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        node.init();
        node.run(); // Этот вызов заблокирует поток до выхода из программы

    } catch (const std::exception& e) {
        std::cerr << "[Fatal Error] " << e.what() << std::endl;
    }

    // 4. Очистка перед выходом
#ifdef _WIN32
    WSACleanup();
#endif
    
    std::cout << "[System] Plexome Node exit success." << std::endl;
    return 0;
}
