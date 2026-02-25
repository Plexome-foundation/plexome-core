#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include "connection_manager.h"
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

namespace plexome {

ConnectionManager::ConnectionManager() 
    : listen_socket_(INVALID_SOCKET), is_running_(false), port_(0) {
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) std::cerr << "[Network] WSAStartup failed: " << res << std::endl;
}

ConnectionManager::~ConnectionManager() {
    stop();
    WSACleanup();
}

bool ConnectionManager::start_server(int port) {
    port_ = port;
    listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket_ == INVALID_SOCKET) {
        std::cerr << "[Network] Socket error: " << WSAGetLastError() << std::endl;
        return false;
    }

    int opt = 1;
    setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons((u_short)port_);

    if (bind(listen_socket_, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "[Network] BIND ERROR: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket_);
        return false;
    }

    if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[Network] LISTEN ERROR: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket_);
        return false;
    }

    // Определяем реальный IP ноды
    char host[256];
    if (gethostname(host, sizeof(host)) == 0) {
        struct hostent* he = gethostbyname(host);
        if (he != nullptr) {
            std::cout << "[Network] Node is broadcasting on:" << std::endl;
            for (int i = 0; he->h_addr_list[i] != nullptr; i++) {
                struct in_addr addr;
                memcpy(&addr, he->h_addr_list[i], sizeof(struct in_addr));
                std::cout << "  -> IP: " << inet_ntoa(addr) << std::endl;
            }
        }
    }

    is_running_ = true;
    std::cout << "[Network] Seed listening on port: " << port_ << std::endl;
    accept_thread_ = std::thread(&ConnectionManager::accept_loop, this);
    return true;
}

void ConnectionManager::accept_loop() {
    while (is_running_) {
        sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        SOCKET s = accept((SOCKET)listen_socket_, (struct sockaddr*)&client_addr, &client_addr_len);
        
        if (s == INVALID_SOCKET) {
            if (is_running_) continue; 
            else break; 
        }

        std::string client_ip = inet_ntoa(client_addr.sin_addr);
        {
            std::lock_guard<std::mutex> lock(sockets_mtx_);
            active_sockets_.push_back((unsigned long long)s);
        }
        std::cout << "\n[Network] [+] Peer " << client_ip << " connected! Total: " << get_active_peers_count() << "\npxm> " << std::flush;
    }
}

bool ConnectionManager::connect_to_seed(const std::string& host, int port) {
    addrinfo hints = {0}, *result = nullptr;
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        std::cerr << "[Network] DNS error for host: " << host << std::endl;
        return false;
    }

    SOCKET s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (s == INVALID_SOCKET) {
        freeaddrinfo(result);
        return false;
    }

    std::cout << "[Network] Attempting to connect to " << host << "..." << std::endl;

    if (connect(s, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        std::cerr << "[Network] Connection failed to " << host << ". Code: " << err << std::endl;
        closesocket(s);
        freeaddrinfo(result);
        return false;
    }

    freeaddrinfo(result);
    {
        std::lock_guard<std::mutex> lock(sockets_mtx_);
        active_sockets_.push_back((unsigned long long)s);
    }
    std::cout << "[Network] Handshake successful with " << host << std::endl;
    return true;
}

size_t ConnectionManager::get_active_peers_count() {
    std::lock_guard<std::mutex> lock(sockets_mtx_);
    return active_sockets_.size();
}

void ConnectionManager::stop() {
    is_running_ = false;
    if (listen_socket_ != INVALID_SOCKET) {
        closesocket((SOCKET)listen_socket_);
        listen_socket_ = INVALID_SOCKET;
    }
    std::lock_guard<std::mutex> lock(sockets_mtx_);
    for (auto s : active_sockets_) {
        shutdown((SOCKET)s, SD_BOTH);
        closesocket((SOCKET)s);
    }
    active_sockets_.clear();
    if (accept_thread_.joinable()) accept_thread_.join();
}

} // namespace plexome
