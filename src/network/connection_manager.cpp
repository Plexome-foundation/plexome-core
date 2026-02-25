#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include "connection_manager.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

namespace plexome {

ConnectionManager::ConnectionManager() 
    : listen_socket_(INVALID_SOCKET), is_running_(false), port_(0) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

ConnectionManager::~ConnectionManager() {
    stop();
    WSACleanup();
}

bool ConnectionManager::start_server(int port) {
    port_ = port;
    // Используем AF_INET6 с поддержкой dual-stack (IPv4 + IPv6)
    listen_socket_ = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket_ == INVALID_SOCKET) return false;

    // Отключаем "только IPv6", чтобы принимать и IPv4 подключения
    int no = 0;
    setsockopt(listen_socket_, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&no, sizeof(no));
    
    int opt = 1;
    setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in6 server_addr = {};
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons((u_short)port_);

    if (bind(listen_socket_, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "[Network] Bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
        return false;
    }

    if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
        return false;
    }

    is_running_ = true;
    std::cout << "[Network] Seed active on port " << port_ << " (Dual Stack IPv4/IPv6)" << std::endl;
    accept_thread_ = std::thread(&ConnectionManager::accept_loop, this);
    return true;
}

void ConnectionManager::accept_loop() {
    while (is_running_) {
        SOCKET s = accept((SOCKET)listen_socket_, NULL, NULL);
        if (s == INVALID_SOCKET) {
            if (is_running_) continue; 
            else break; 
        }
        {
            std::lock_guard<std::mutex> lock(sockets_mtx_);
            active_sockets_.push_back((unsigned long long)s);
        }
        std::cout << "\n[Network] Incoming connection established! Total: " << get_active_peers_count() << "\npxm> " << std::flush;
    }
}

bool ConnectionManager::connect_to_seed(const std::string& host, int port) {
    addrinfo hints = {0}, *result = nullptr;
    hints.ai_family = AF_UNSPEC; // Авто-выбор IPv4 или IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int res = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);
    if (res != 0) {
        std::cerr << "[Network] DNS Lookup failed: " << host << " (Error: " << res << ")" << std::endl;
        return false;
    }

    SOCKET s = INVALID_SOCKET;
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (s == INVALID_SOCKET) continue;

        if (connect(s, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
            int err = WSAGetLastError();
            // 10061 = Connection Refused (порт закрыт или сервер не запущен)
            // 10060 = Timed Out (брандмауэр блокирует пакеты)
            std::cerr << "[Network] Connect to " << host << " failed. OS Error: " << err << std::endl;
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
        active_sockets_.push_back((unsigned long long)s);
    }
    std::cout << "[Network] Handshake complete with " << host << std::endl;
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
        shutdown((SOCKET)s, 2); 
        closesocket((SOCKET)s);
    }
    active_sockets_.clear();
    if (accept_thread_.joinable()) accept_thread_.join();
}

} // namespace plexome
