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
#ifdef _WIN32
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) std::cerr << "[Network] WSAStartup failed: " << res << std::endl;
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
    if (listen_socket_ == INVALID_SOCKET) {
        std::cerr << "[Network] Socket creation failed: " << WSAGetLastError() << std::endl;
        return false;
    }

    int opt = 1;
    setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons((u_short)port_);

    if (bind(listen_socket_, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "[Network] BIND ERROR (Port " << port_ << "): " << WSAGetLastError() << std::endl;
        closesocket(listen_socket_);
        return false;
    }

    if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[Network] LISTEN ERROR: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket_);
        return false;
    }

    // Получаем локальный IP для вывода в консоль
    char host_name[256];
    char local_ip[INET_ADDRSTRLEN] = "127.0.0.1";
    if (gethostname(host_name, sizeof(host_name)) == 0) {
        struct addrinfo hints = {}, *res = nullptr;
        hints.ai_family = AF_INET;
        if (getaddrinfo(host_name, nullptr, &hints, &res) == 0) {
            sockaddr_in* addr = (sockaddr_in*)res->ai_addr;
            inet_ntop(AF_INET, &addr->sin_addr, local_ip, INET_ADDRSTRLEN);
            freeaddrinfo(res);
        }
    }

    is_running_ = true;
    std::cout << "[Network] Seed logic active!" << std::endl;
    std::cout << "[Network] Local IP: " << local_ip << std::endl;
    std::cout << "[Network] Port:     " << port_ << std::endl;
    std::cout << "[Network] Waiting for peers..." << std::endl;

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

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        {
            std::lock_guard<std::mutex> lock(sockets_mtx_);
            active_sockets_.push_back((unsigned long long)s);
        }
        std::cout << "\n[Network] [+] New peer connected from " << client_ip 
                  << "! Total peers: " << get_active_peers_count() << "\npxm> " << std::flush;
    }
}

bool ConnectionManager::connect_to_seed(const std::string& host, int port) {
    addrinfo hints = {0}, *result = nullptr;
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int dns_res = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);
    if (dns_res != 0) {
        std::cerr << "[Network] DNS ERROR for " << host << ": " << dns_res << std::endl;
        return false;
    }

    SOCKET s = INVALID_SOCKET;
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (s == INVALID_SOCKET) continue;

        if (connect(s, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
            int err = WSAGetLastError();
            std::cerr << "[Network] CONNECT ERROR to " << host << " (" << host << "): " << err;
            if (err == 10061) std::cerr << " (Refused)";
            if (err == 10060) std::cerr << " (Timeout)";
            std::cerr << std::endl;
            
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
    std::cout << "[Network] Connected to Swarm Seed: " << host << std::endl;
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
