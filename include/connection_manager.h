#ifndef PLEXOME_CONNECTION_MANAGER_H
#define PLEXOME_CONNECTION_MANAGER_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

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

    unsigned long long listen_socket_; 
    std::vector<unsigned long long> active_sockets_;
    std::mutex sockets_mtx_;
    std::thread accept_thread_;
    std::atomic<bool> is_running_;
    int port_;
};

} // namespace plexome

#endif
