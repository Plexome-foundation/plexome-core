#pragma once
#include "plexome_types.h"
#include <set>
#include <string>
#include <mutex>

namespace plexome {

    /**
     * Epidemic protocol for state propagation.
     * Ensures all nodes eventually receive network-wide updates.
     */
    class GossipService {
    public:
        GossipService();

        // Register a message as "seen" to prevent infinite loops
        bool process_message(const std::string& message_id);

        // Select random peers to forward the message to
        std::vector<PeerID> select_targets(const std::vector<PeerID>& available_peers, size_t fanout = 3);

    private:
        std::set<std::string> seen_messages_;
        mutable std::mutex mtx_;
    };
}
