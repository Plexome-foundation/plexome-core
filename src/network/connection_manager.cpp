#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include "connection_manager.h"
#include <iostream>
#include <string>
#include <vector>

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
    if (listen_socket_ == INVALID_SOCKET) return false;

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
        closesocket(listen_socket_);
        return false;
    }

    // Вывод всех IP-адресов сервера для проверки
    char host_name[256];
    if (gethostname(host_name, sizeof(host_name)) == 0) {
        struct addrinfo hints = {}, *res = nullptr;
        hints.ai_family = AF_INET;
        if (getaddrinfo(host_name, nullptr, &hints, &res) == 0) {
            std::cout << "[Network] Server is listening on interfaces:" << std::endl;
            for (auto p = res; p != nullptr; p = p->ai_next) {
                char ip_str[INET_ADDRSTRLEN];
                struct sockaddr_in* addr = (struct sockaddr_in*)p->ai_addr;
                inet_ntop(AF_INET, &addr->sin_addr, ip_str, sizeof(ip_str));
                std::cout << "  -> IP: " << ip_str << std::endl;
            }
            freeaddrinfo(res);
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
        int addr_len = sizeof(client_addr);
        SOCKET s = accept((SOCKET)listen_socket_, (struct sockaddr*)&client_addr, &addr_len);
        
        if (s == INVALID_SOCKET) {
            if (is_running_) continue; 
            else break; 
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

        {
            std::lock_guard<std::mutex> lock(sockets_mtx_);
            active_sockets_.push_back((unsigned long long)s);
        }
        std::cout << "\n[Network] [+] Peer connected: " << client_ip << " | Total: " << get_active_peers_count() << "\npxm> " << std::flush;
    }
}

bool ConnectionManager::connect_to_seed(const std::string& host, int port) {
    addrinfo hints = {0}, *result = nullptr;
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        std::cerr << "[Network] DNS error: " << host << std::endl;
        return false;
    }

    SOCKET s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (s == INVALID_SOCKET) {
        freeaddrinfo(result);
        return false;
    }

    if (connect(s, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        std::cerr << "[Network] Connection to " << host << " failed! Error: " << err;
        if (err == 10060) std::cerr << " (Timeout - Check Firewall)";
        if (err == 10061) std::cerr << " (Refused - Check if Server is running)";
        std::cerr << std::endl;
        closesocket(s);
        freeaddrinfo(result);
        return false;
    }

    freeaddrinfo(result);
    {
        std::lock_guard<std::mutex> lock(sockets_mtx_);
        active_sockets_.push_back((unsigned long long)s);
    }
    std::cout << "[Network] Connected to " << host << std::endl;
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
