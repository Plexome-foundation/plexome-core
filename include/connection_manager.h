#include "connection_manager.h"
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

namespace plexome {

ConnectionManager::ConnectionManager() : listen_socket_(INVALID_SOCKET), is_running_(false) {
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
    port_ = port;
    listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket_ == INVALID_SOCKET) return false;

    int opt = 1;
    setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
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
        std::cout << "\n[Network] New peer joined swarm! Total: " << active_sockets_.size() << "\npxm> " << std::flush;
    }
}

bool ConnectionManager::connect_to_seed(const std::string& host, int port) {
    addrinfo hints = {0}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        return false;
    }

    SOCKET s = INVALID_SOCKET;
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (s == INVALID_SOCKET) continue;

        if (connect(s, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
            closesocket(s);
            s = INVALID_SOCKET;
            continue;
        }
        break; 
    }

    freeaddrinfo(result);

    if (s == INVALID_SOCKET) return false;

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

} // namespace plexome
