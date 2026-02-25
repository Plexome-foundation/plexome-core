#pragma once
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace plexome {
    class ConnectionManager {
    public:
        ConnectionManager();
        ~ConnectionManager();

        bool start_server(int port);
        bool connect_to_seed(const std::string& host, int port);
        size_t get_active_peers_count();
        void stop();

    private:
        void accept_loop();

        std::atomic<bool> is_running_{false};
        int port_{7539};
        
#ifdef _WIN32
        SOCKET listen_socket_{INVALID_SOCKET};
        std::vector<SOCKET> active_sockets_;
#endif
        std::mutex sockets_mtx_;
        std::thread accept_thread_;
    };
}
