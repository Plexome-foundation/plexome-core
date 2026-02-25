#include "connection_manager.h"
#include <iostream>
#include <chrono>

namespace plexome {

    ConnectionManager::ConnectionManager() {
        std::cout << "[Network] Connection Manager initialized." << std::endl;
    }

    bool ConnectionManager::handle_handshake(const HandshakePacket& packet) {
        std::lock_guard<std::mutex> lock(mtx_);

        if (packet.version != PROTOCOL_VERSION) {
            std::cerr << "[Handshake] Version mismatch! Peer: " << packet.sender_id 
                      << " uses v" << packet.version << std::endl;
            return false;
        }

        PeerSession session;
        session.id = packet.sender_id;
        session.role = packet.sender_role;
        session.is_authenticated = true; // Later: verify signature
        session.last_seen = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        active_sessions_[packet.sender_id] = session;
        
        std::cout << "[Handshake] Peer " << packet.sender_id 
                  << " authenticated as " << (session.role == NodeRole::Titan ? "TITAN" : "ARCHIVIST") 
                  << std::endl;
        
        return true;
    }

    bool ConnectionManager::is_peer_trusted(const PeerID& id) const {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = active_sessions_.find(id);
        return (it != active_sessions_.end() && it->second.is_authenticated);
    }

    void ConnectionManager::evict_peer(const PeerID& id) {
        std::lock_guard<std::mutex> lock(mtx_);
        active_sessions_.erase(id);
    }
}
