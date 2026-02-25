#include "bootstrap_manager.h"
#include <iostream>

namespace plexome {

    BootstrapManager::BootstrapManager(bool is_seed) 
        : is_seed_node_(is_seed) {
        dns_discovery_ = std::make_unique<DNSDiscovery>();
    }

    void BootstrapManager::bootstrap() {
        if (is_seed_node_) {
            std::cout << "[Bootstrap] Running in SEED MODE. Waiting for incoming connections..." << std::endl;
            return;
        }

        std::cout << "[Bootstrap] Running in CLIENT MODE. Discovering swarm..." << std::endl;
        auto seed_ips = dns_discovery_->resolve_seeds();
        
        for (const auto& ip : seed_ips) {
            std::cout << "[Bootstrap] Found seed node: " << ip << ". Attempting handshake..." << std::endl;
            register_peer(ip);
        }
    }

    void BootstrapManager::register_peer(const std::string& ip) {
        // Prevent duplicates
        for (const auto& p : peer_directory_) if (p == ip) return;
        
        peer_directory_.push_back(ip);
        std::cout << "[Bootstrap] Peer registered: " << ip << " (Total: " << peer_directory_.size() << ")" << std::endl;
    }

    std::vector<std::string> BootstrapManager::get_known_peers() const {
        return peer_directory_;
    }
}
