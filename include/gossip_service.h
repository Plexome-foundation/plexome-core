#pragma once
#include "plexome_types.h"
#include <vector>

namespace plexome {

    class GossipService {
    public:
        GossipService() = default;

        // Starts the background peer discovery process
        void start_gossip_protocol();

        // Safely stops the gossip background tasks
        void stop_gossip_protocol();

        // Selects a random subset of peers for the next gossip round
        std::vector<PeerID> get_random_peers(size_t count, const std::vector<PeerID>& all_known_peers);

    private:
        bool is_running_ = false;
    };
}
