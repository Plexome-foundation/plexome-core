/**
 * Plexome Core v2.0 - Network Module (DNS Bootstrap & DHT Foundation)
 * Author: Georgii
 * Implements DNS seed resolution and basic Peer Exchange (PEX) for swarm building.
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")

#include "../../include/module_interface.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <set>
#include <mutex>
#include <sstream>

struct NetworkState {
    SOCKET listen_socket = INVALID_SOCKET;
    std::atomic<bool> is_running = false;
    std::thread server_thread;
    std::thread dht_thread; 
    
    PerformanceTier current_tier{};
    int my_port = 4242;

    // DHT Foundation: Routing Table to store known peer IPs
    std::set<std::string> routing_table;
    std::mutex dht_mutex;
};

static NetworkState g_state;

/**
 * Utility to split strings (useful for parsing peer lists)
 */
std::vector<std::string> split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

/**
 * Resolves DNS seed to get initial peer IPs and adds them to the routing table.
 */
void dns_bootstrap(const std::string& seed_domain) {
    std::cout << "[Network] Bootstrapping from DNS seed: " << seed_domain << "..." << std::endl;
    
    addrinfo hints = {};
    hints.ai_family = AF_INET; // Force IPv4 for simplicity right now
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    if (getaddrinfo(seed_domain.c_str(), "4242", &hints, &result) != 0) {
        std::cerr << "[Network] DNS resolution failed for " << seed_domain << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(g_state.dht_mutex);
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        sockaddr_in* sockaddr_ipv4 = reinterpret_cast<sockaddr_in*>(ptr->ai_addr);
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ip_str, INET_ADDRSTRLEN);
        
        g_state.routing_table.insert(ip_str);
        std::cout << "[Network] Discovered Seed Node via DNS: " << ip_str << std::endl;
    }
    freeaddrinfo(result);
}

/**
 * Server Loop: Handles incoming connections and serves PEX requests.
 */
void server_loop() {
    while (g_state.is_running) {
        sockaddr_in client_addr;
        int client_size = sizeof(client_addr);
        SOCKET client_socket = accept(g_state.listen_socket, (sockaddr*)&client_addr, &client_size);
        
        if (client_socket == INVALID_SOCKET) continue;

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);

        // Add incoming peer to our routing table
        {
            std::lock_guard<std::mutex> lock(g_state.dht_mutex);
            g_state.routing_table.insert(ip_str);
        }

        char recv_buf[512] = {0};
        int bytes = recv(client_socket, recv_buf, sizeof(recv_buf) - 1, 0);
        
        if (bytes > 0) {
            std::string req(recv_buf);
            
            // Handle DHT Peer Exchange Request
            if (req.find("GET_PEERS") != std::string::npos) {
                std::string response = "PEER_LIST|";
                std::lock_guard<std::mutex> lock(g_state.dht_mutex);
                for (const auto& ip : g_state.routing_table) {
                    response += ip + ",";
                }
                response += "\n";
                send(client_socket, response.c_str(), (int)response.length(), 0);
                std::cout << "[Network] Served routing table to " << ip_str << std::endl;
            } else {
                // Standard handshake response
                std::string response = "PLEXOME_NODE_V2|TIER:TITAN\n";
                send(client_socket, response.c_str(), (int)response.length(), 0);
            }
        }
        closesocket(client_socket);
    }
}

/**
 * DHT Maintenance Loop: Connects to known peers to discover more peers.
 */
void dht_maintenance_loop() {
    std::cout << "[Network] DHT Maintenance thread active." << std::endl;
    
    // 1. Initial DNS Bootstrap
    dns_bootstrap("plexome.ai"); 

    while (g_state.is_running) {
        Sleep(10000); // Run maintenance every 10 seconds

        std::vector<std::string> current_peers;
        {
            std::lock_guard<std::mutex> lock(g_state.dht_mutex);
            current_peers.assign(g_state.routing_table.begin(), g_state.routing_table.end());
        }

        for (const auto& peer_ip : current_peers) {
            if (!g_state.is_running) break;

            // Don't connect to ourselves (assuming testing on localhost for now)
            // In production, you'd check against local interface IPs
            if (peer_ip == "127.0.0.1") continue; 

            SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock == INVALID_SOCKET) continue;

            // 1-second timeout for connection attempts so we don't hang
            DWORD timeout = 1000;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

            sockaddr_in target_addr;
            target_addr.sin_family = AF_INET;
            inet_pton(AF_INET, peer_ip.c_str(), &target_addr.sin_addr);
            target_addr.sin_port = htons(4242);

            // Ask the peer for their routing table
            if (connect(sock, (sockaddr*)&target_addr, sizeof(target_addr)) != SOCKET_ERROR) {
                std::string req = "GET_PEERS\n";
                send(sock, req.c_str(), (int)req.length(), 0);

                char buf[1024] = {0};
                if (recv(sock, buf, sizeof(buf)-1, 0) > 0) {
                    std::string resp(buf);
                    if (resp.find("PEER_LIST|") == 0) {
                        auto ips = split_string(resp.substr(10), ',');
                        std::lock_guard<std::mutex> lock(g_state.dht_mutex);
                        for (const auto& new_ip : ips) {
                            if (!new_ip.empty() && new_ip != "\n") {
                                g_state.routing_table.insert(new_ip);
                            }
                        }
                    }
                }
            }
            closesocket(sock);
        }
    }
}

extern "C" {
    PXM_API PxmModuleInfo pxm_get_info() {
        return { "Plexome P2P Network", "3.0.0-dht-seed", "DNS Bootstrap and DHT routing table module." };
    }

    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        if (!config) return PxmStatus::ERROR_INIT_FAILED;
        g_state.current_tier = config->tier;

        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return PxmStatus::ERROR_INIT_FAILED;

        g_state.listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (g_state.listen_socket == INVALID_SOCKET) return PxmStatus::ERROR_INIT_FAILED;

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(g_state.my_port); 

        if (bind(g_state.listen_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "[Network] Bind failed. Port 4242 in use." << std::endl;
            closesocket(g_state.listen_socket);
            WSACleanup();
            return PxmStatus::ERROR_INIT_FAILED;
        }

        if (listen(g_state.listen_socket, SOMAXCONN) == SOCKET_ERROR) return PxmStatus::ERROR_INIT_FAILED;

        g_state.is_running = true;
        g_state.server_thread = std::thread(server_loop);
        g_state.dht_thread = std::thread(dht_maintenance_loop); // Start DHT logic

        std::cout << "[Network] P2P Node active on port " << g_state.my_port << "." << std::endl;
        return PxmStatus::OK;
    }

    PXM_API void pxm_shutdown() {
        std::cout << "[Network] Shutting down P2P network..." << std::endl;
        g_state.is_running = false;

        if (g_state.listen_socket != INVALID_SOCKET) {
            closesocket(g_state.listen_socket);
            g_state.listen_socket = INVALID_SOCKET;
        }

        if (g_state.server_thread.joinable()) g_state.server_thread.join();
        if (g_state.dht_thread.joinable()) g_state.dht_thread.join();

        WSACleanup();
    }
}
