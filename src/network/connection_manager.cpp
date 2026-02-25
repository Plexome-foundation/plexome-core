#include "connection_manager.h"
#include <iostream>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

namespace plexome {

    ConnectionManager::ConnectionManager() {
#ifdef _WIN32
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "[Network] WSAStartup failed: " << result << "\n";
        }
#endif
    }

    ConnectionManager::~ConnectionManager() {
        stop();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    bool ConnectionManager::start_server(int port) {
#ifdef _WIN32
        port_ = port;
        listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_socket_ == INVALID_SOCKET) return false;

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port_);

        if (bind(listen_socket_, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            closesocket(listen_socket_);
            return false;
        }

        if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(listen_socket_);
            return false;
        }

        is_running_ = true;
        std::cout << "[Network] Seed node listening on port " << port_ << "...\n";
        accept_thread_ = std::thread(&ConnectionManager::accept_loop, this);
        return true;
#else
        return false;
#endif
    }

    void ConnectionManager::accept_loop() {
#ifdef _WIN32
        while (is_running_) {
            SOCKET client_socket = accept(listen_socket_, NULL, NULL);
            if (client_socket != INVALID_SOCKET) {
                std::lock_guard<std::mutex> lock(sockets_mtx_);
                active_sockets_.push_back(client_socket);
                // Выводим уведомление, не ломая строку pxm>
                std::cout << "\n[Network] [+] New peer connected! Total: " << active_sockets_.size() << "\npxm> " << std::flush;
            }
        }
#endif
    }

    bool ConnectionManager::connect_to_seed(const std::string& host, int port) {
#ifdef _WIN32
        addrinfo hints = {0}, *result = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
            std::cerr << "[Network] DNS error for " << host << "\n";
            return false;
        }

        SOCKET s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (connect(s, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
            closesocket(s);
            freeaddrinfo(result);
            return false;
        }

        freeaddrinfo(result);
        std::lock_guard<std::mutex> lock(sockets_mtx_);
        active_sockets_.push_back(s);
        std::cout << "[Network] Connected to Swarm via DNS.\n";
        return true;
#endif
        return false;
    }

    size_t ConnectionManager::get_active_peers_count() {
        std::lock_guard<std::mutex> lock(sockets_mtx_);
        return active_sockets_.size();
    }

    void ConnectionManager::stop() {
        is_running_ = false;
#ifdef _WIN32
        if (listen_socket_ != INVALID_SOCKET) {
            closesocket(listen_socket_);
            listen_socket_ = INVALID_SOCKET;
        }
        std::lock_guard<std::mutex> lock(sockets_mtx_);
        for (SOCKET s : active_sockets_) closesocket(s);
        active_sockets_.clear();
#endif
        if (accept_thread_.joinable()) accept_thread_.join();
    }
}
