#ifndef PLEXOME_CONNECTION_MANAGER_H
#define PLEXOME_CONNECTION_MANAGER_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>

namespace plexome {

class ConnectionManager {
public:
    ConnectionManager();
    ~ConnectionManager();

    bool start_server(int port);
    bool connect_to_seed(const std::string& host, int port);
    size_t get_active_peers_count();
    void stop();

    // Отправка и получение сообщений
    void broadcast(const std::string& message);
    bool get_next_message(std::string& out_msg);

private:
    void accept_loop();
    void receive_loop(); // НОВЫЙ ПОТОК: слушает сеть

    unsigned long long listen_socket_; 
    std::vector<unsigned long long> active_sockets_;
    std::mutex sockets_mtx_;
    
    std::thread accept_thread_;
    std::thread receive_thread_;
    std::atomic<bool> is_running_;
    int port_;

    std::queue<std::string> message_queue_;
    std::mutex queue_mtx_;
};

} // namespace plexome

#endif
