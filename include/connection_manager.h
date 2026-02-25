#include "connection_manager.h"
#include <iostream>
#include <string>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

namespace plexome {

    ConnectionManager::ConnectionManager() : listen_socket_(INVALID_SOCKET), is_running_(false) {
        // Инициализация здесь гарантирует, что сеть готова до любого вызова
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }

    ConnectionManager::~ConnectionManager() {
        stop();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    bool ConnectionManager::start_server(int port) {
        port_ = port;
        listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_socket_ == INVALID_SOCKET) return false;

        // Позволяет сокету принудительно занять порт, если он в состоянии TIME_WAIT
        int opt = 1;
        setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY; // Слушаем все сетевые интерфейсы
        server_addr.sin_port = htons((u_short)port_);

        if (bind(listen_socket_, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            closesocket(listen_socket_);
            return false;
        }

        if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(listen_socket_);
            return false;
        }

        is_running_ = true;
        std::cout << "[Network] Seed logic active on port " << port_ << "..." << std::endl;
        accept_thread_ = std::thread(&ConnectionManager::accept_loop, this);
        return true;
    }

    void ConnectionManager::accept_loop() {
        while (is_running_) {
            SOCKET client_socket = accept(listen_socket_, NULL, NULL);
            
            if (client_socket == INVALID_SOCKET) {
                if (is_running_) continue; 
                else break; 
            }

            std::lock_guard<std::mutex> lock(sockets_mtx_);
            active_sockets_.push_back(client_socket);
            // Печатаем уведомление о новом пире
            std::cout << "\n[Network] New peer joined swarm! Total: " << active_sockets_.size() << "\npxm> " << std::flush;
        }
    }

    bool ConnectionManager::connect_to_seed(const std::string& host, int port) {
        addrinfo hints = {0}, *result = nullptr;
        hints.ai_family = AF_INET; // Используем IPv4
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        // DNS резолвинг (твоя инфраструктура подставит IP)
        if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
            return false;
        }

        SOCKET s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (s == INVALID_SOCKET) {
            freeaddrinfo(result);
            return false;
        }

        // Попытка соединения
        if (connect(s, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
            closesocket(s);
            freeaddrinfo(result);
            return false;
        }

        freeaddrinfo(result);
        
        {
            std::lock_guard<std::mutex> lock(sockets_mtx_);
            active_sockets_.push_back(s);
        }
        
        std::cout << "[Network] Successfully connected to seed: " << host << std::endl;
        return true;
    }

    size_t ConnectionManager::get_active_peers_count() {
        std::lock_guard<std::mutex> lock(sockets_mtx_);
        return active_sockets_.size();
    }

    void ConnectionManager::stop() {
        is_running_ = false;
        if (listen_socket_ != INVALID_SOCKET) {
            closesocket(listen_socket_);
            listen_socket_ = INVALID_SOCKET;
        }
        
        std::lock_guard<std::mutex> lock(sockets_mtx_);
        for (SOCKET s : active_sockets_) {
            shutdown(s, SD_BOTH);
            closesocket(s);
        }
        active_sockets_.clear();

        if (accept_thread_.joinable()) {
            accept_thread_.join();
        }
    }
}
