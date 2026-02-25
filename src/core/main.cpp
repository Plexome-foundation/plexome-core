#include "node.h"
#include "plexome_types.h"
#include <iostream>
#include <string>
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
    plexome::AppConfig config;
    config.port = 7539;
    config.seed_host = "seed1.plexome.ai";
    config.storage_path = "./data";

    // Базовые настройки для клиента (PEER)
    config.is_seed = false; 
    config.node_id = "PXM-WORKER";

    // 3. ПЕРЕХВАТ КОМАНДНОЙ СТРОКИ
    // Проверяем, не запустили ли нас с флагом --seed
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--seed") {
            config.is_seed = true;
            config.node_id = "PXM-SEED-SERVER";
        }
    }

    // Если ты не умеешь/не хочешь запускать программу через консоль с аргументами,
    // и просто кликаешь по .exe на сервере, раскомментируй строку ниже перед сборкой:
    // config.is_seed = true;

    // Выводим в консоль, кем мы в итоге стали
    if (config.is_seed) {
        std::cout << "========================================\n";
        std::cout << "[System] LAUNCHING AS SEED (SERVER)\n";
        std::cout << "========================================\n";
    } else {
        std::cout << "========================================\n";
        std::cout << "[System] LAUNCHING AS PEER (CLIENT)\n";
        std::cout << "========================================\n";
    }

    try {
        // 4. Создание и запуск ноды
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

    // 5. Очистка перед выходом
#ifdef _WIN32
    WSACleanup();
#endif
    
    std::cout << "[System] Plexome Node exit success." << std::endl;
    return 0;
}
