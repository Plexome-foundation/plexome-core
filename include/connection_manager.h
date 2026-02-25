#pragma once
#include "plexome_types.h"
#include "protocol.h"
#include <map>
#include <mutex>

namespace plexome {

    struct PeerSession {
        PeerID id;
        NodeRole role;
        bool is_authenticated;
        uint64_t last_seen;
    };

    /**
     * Manages active network connections and their handshake status.
     */
    class ConnectionManager {
    public:
        ConnectionManager();

        // Process an incoming handshake from a new connection
        bool handle_handshake(const HandshakePacket& packet);

        // Check if a peer is active and trusted
        bool is_peer_trusted(const PeerID& id) const;

        // Disconnect a peer
        void evict_peer(const PeerID& id);

    private:
        std::map<PeerID, PeerSession> active_sessions_;
        mutable std::mutex mtx_;
    };
}
