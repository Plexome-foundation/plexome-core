#include "gossip_service.h"
#include <iostream>
#include <random>

namespace plexome {

    GossipService::GossipService() {
        std::cout << "[Gossip] Service initialized." << std::endl;
    }

    bool GossipService::process_message(const std::string& message_id) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (seen_messages_.find(message_id) != seen_messages_.end()) {
            return false; // Already seen
        }
        seen_messages_.insert(message_id);
        return true; // New message
    }

    std::vector<PeerID> GossipService::select_targets(const std::vector<PeerID>& peers, size_t fanout) {
        if (peers.empty()) return {};
        
        std::vector<PeerID> targets;
        std::vector<PeerID> shuffled_peers = peers;
        
        std::random_device rd;
        std::mtf19937 g(rd());
        std::shuffle(shuffled_peers.begin(), shuffled_peers.end(), g);

        for (size_t i = 0; i < std::min(fanout, shuffled_peers.size()); ++i) {
            targets.push_back(shuffled_peers[i]);
        }
        return targets;
    }
}
