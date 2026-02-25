#include "gossip_service.h"
#include <iostream>
#include <algorithm>
#include <random> // КРИТИЧНО: Убедились, что random подключен

namespace plexome {

    void GossipService::start_gossip_protocol() {
        std::cout << "[Network] Initiating Gossip Protocol for peer discovery..." << std::endl;
        is_running_ = true;
        // In a real implementation, this would spin up a background thread
    }

    void GossipService::stop_gossip_protocol() {
        is_running_ = false;
        std::cout << "[Network] Gossip Protocol stopped." << std::endl;
    }

    std::vector<PeerID> GossipService::get_random_peers(size_t count, const std::vector<PeerID>& all_known_peers) {
        if (all_known_peers.empty() || count == 0) return {};

        std::vector<PeerID> selected_peers = all_known_peers;
        
        // Исправлена опечатка: mt19937 вместо mtf19937
        std::random_device rd;
        std::mt19937 g(rd());
        
        std::shuffle(selected_peers.begin(), selected_peers.end(), g);
        
        if (selected_peers.size() > count) {
            selected_peers.resize(count);
        }
        
        return selected_peers;
    }
}
