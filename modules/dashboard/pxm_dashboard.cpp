/**
 * Plexome Core v2.0 - Dashboard Module (Web UI Edition)
 * Author: Georgii
 * Implements an HTTP server on port 8080 to serve node statistics to a web browser.
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

struct DashboardState {
    SOCKET listen_socket = INVALID_SOCKET;
    std::atomic<bool> is_running = false;
    std::thread server_thread;
    PerformanceTier current_tier{};
};

static DashboardState g_state;

/**
 * Generates the HTML layout for the dashboard.
 */
std::string generate_html() {
    std::string tier_str = (g_state.current_tier >= PerformanceTier::TITAN) ? "TITAN" : "STANDARD";
    
    return "<html><head><title>Plexome Node Dashboard</title>"
           "<style>"
           "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #0d1117; color: #c9d1d9; padding: 40px; }"
           "h1 { color: #58a6ff; }"
           ".card { background-color: #161b22; border: 1px solid #30363d; border-radius: 6px; padding: 20px; max-width: 400px; }"
           ".status-green { color: #3fb950; font-weight: bold; }"
           ".tier-badge { background-color: #ff7b72; color: #0d1117; padding: 2px 8px; border-radius: 12px; font-weight: bold; font-size: 0.9em; }"
           "</style></head><body>"
           "<h1>Plexome Core Engine</h1>"
           "<div class='card'>"
           "<h3>Node Statistics</h3>"
           "<p>System Status: <span class='status-green'>ONLINE</span></p>"
           "<p>Performance Tier: <span class='tier-badge'>" + tier_str + "</span></p>"
           "<p>AI Module: <span class='status-green'>ACTIVE (Phi-3)</span></p>"
           "<p>P2P Network Port: <b>4242</b></p>"
           "</div>"
           "</body></html>";
}

/**
 * Background thread loop to handle HTTP requests.
 */
void dashboard_loop() {
    std::cout << "[Dashboard] Web UI listening on http://localhost:8080" << std::endl;

    while (g_state.is_running) {
        sockaddr_in client_addr;
        int client_size = sizeof(client_addr);
        SOCKET client_socket = accept(g_state.listen_socket, (sockaddr*)&client_addr, &client_size);
        
        if (client_socket == INVALID_SOCKET) continue;

        // Read the incoming HTTP GET request
        char recv_buf[1024] = {0};
        recv(client_socket, recv_buf, sizeof(recv_buf) - 1, 0);

        // Generate HTTP response with HTML body
        std::string html_body = generate_html();
        std::string http_response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Connection: close\r\n"
            "Content-Length: " + std::to_string(html_body.length()) + "\r\n"
            "\r\n" + html_body;

        send(client_socket, http_response.c_str(), (int)http_response.length(), 0);
        closesocket(client_socket);
    }
}

extern "C" {
    PXM_API PxmModuleInfo pxm_get_info() {
        return { "Plexome Web Dashboard", "2.0.0", "HTTP server for local node monitoring." };
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
        server_addr.sin_port = htons(8080); // HTTP Port

        if (bind(g_state.listen_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            closesocket(g_state.listen_socket);
            WSACleanup();
            return PxmStatus::ERROR_INIT_FAILED;
        }

        if (listen(g_state.listen_socket, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(g_state.listen_socket);
            WSACleanup();
            return PxmStatus::ERROR_INIT_FAILED;
        }

        g_state.is_running = true;
        g_state.server_thread = std::thread(dashboard_loop);

        return PxmStatus::OK;
    }

    PXM_API void pxm_shutdown() {
        std::cout << "[Dashboard] Shutting down Web UI..." << std::endl;
        g_state.is_running = false;
        if (g_state.listen_socket != INVALID_SOCKET) {
            closesocket(g_state.listen_socket);
            g_state.listen_socket = INVALID_SOCKET;
        }
        if (g_state.server_thread.joinable()) {
            g_state.server_thread.join();
        }
        WSACleanup();
    }
}
