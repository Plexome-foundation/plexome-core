/**
 * Plexome Core v2.0 - Network Module (P2P Handshake Edition)
 * Author: Georgii
 * Implements a background TCP listener for node discovery and basic handshakes.
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib") // Tell MSVC to link the Winsock library

#include "../../include/module_interface.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <vector>

// Internal state for the network module
struct NetworkState {
    SOCKET listen_socket = INVALID_SOCKET;
    std::atomic<bool> is_running = false;
    std::thread server_thread;
    PerformanceTier current_tier = PerformanceTier::UNKNOWN;
};

static NetworkState g_state;

/**
 * Background thread loop to accept incoming P2P connections.
 */
void server_loop() {
    std::cout << "[Network] P2P Listener thread started on port 4242." << std::endl;

    while (g_state.is_running) {
        sockaddr_in client_addr;
        int client_size = sizeof(client_addr);
        
        // Accept incoming connection
        SOCKET client_socket = accept(g_state.listen_socket, (sockaddr*)&client_addr, &client_size);
        
        if (client_socket == INVALID_SOCKET) {
            if (g_state.is_running) {
                std::cerr << "[Network] Accept failed: " << WSAGetLastError() << std::endl;
            }
            continue;
        }

        // We got a connection! Get the IP address
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
        std::cout << "\n[Network] Incoming P2P connection from " << ip_str << std::endl;

        // 1. Read greeting from the peer
        char recv_buf[512] = {0};
        int bytes_received = recv(client_socket, recv_buf, sizeof(recv_buf) - 1, 0);
        if (bytes_received > 0) {
            std::cout << "[Network] Peer says: " << recv_buf << std::endl;
        }

        // 2. Send our handshake back
        std::string my_tier = (g_state.current_tier == PerformanceTier::TITAN) ? "TITAN" : "STANDARD";
        std::string handshake_msg = "PLEXOME_NODE_V2|TIER:" + my_tier + "\n";
        
        send(client_socket, handshake_msg.c_str(), (int)handshake_msg.length(), 0);

        // Close connection for now (simple one-off handshake)
        closesocket(client_socket);
        std::cout << "[Network] Handshake complete. Connection closed.\n>>> " << std::flush;
    }
}

extern "C" {
    PXM_API PxmModuleInfo pxm_get_info() {
        return { "Plexome P2P Network", "2.1.0-handshake", "TCP listener for peer discovery." };
    }

    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        if (!config) return PxmStatus::ERROR_INIT_FAILED;
        
        g_state.current_tier = config->tier;

        // 1. Initialize Winsock
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "[Network] WSAStartup failed: " << result << std::endl;
            return PxmStatus::ERROR_INIT_FAILED;
        }

        // 2. Create listening socket
        g_state.listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (g_state.listen_socket == INVALID_SOCKET) {
            std::cerr << "[Network] Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return PxmStatus::ERROR_INIT_FAILED;
        }

        // 3. Bind socket to port 4242
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
        server_addr.sin_port = htons(4242);

        if (bind(g_state.listen_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "[Network] Bind failed (Port 4242 might be in use): " << WSAGetLastError() << std::endl;
            closesocket(g_state.listen_socket);
            WSACleanup();
            return PxmStatus::ERROR_INIT_FAILED;
        }

        // 4. Start listening
        if (listen(g_state.listen_socket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "[Network] Listen failed: " << WSAGetLastError() << std::endl;
            closesocket(g_state.listen_socket);
            WSACleanup();
            return PxmStatus::ERROR_INIT_FAILED;
        }

        // 5. Launch background thread
        g_state.is_running = true;
        g_state.server_thread = std::thread(server_loop);

        std::cout << "[Network] P2P Stack initialized on port 4242." << std::endl;
        return PxmStatus::OK;
    }

    PXM_API void pxm_shutdown() {
        std::cout << "[Network] Shutting down P2P stack..." << std::endl;
        g_state.is_running = false;

        // Force close the listening socket to break the accept() blocking call
        if (g_state.listen_socket != INVALID_SOCKET) {
            closesocket(g_state.listen_socket);
            g_state.listen_socket = INVALID_SOCKET;
        }

        // Wait for the server thread to finish safely
        if (g_state.server_thread.joinable()) {
            g_state.server_thread.join();
        }

        WSACleanup();
        std::cout << "[Network] Network module offline." << std::endl;
    }
}
