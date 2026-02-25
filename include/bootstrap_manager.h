#pragma once
#include "plexome_types.h"
#include "dns_discovery.h"
#include <vector>
#include <memory>

namespace plexome {

    /**
     * Manages the lifecycle of joining the network.
     * If is_seed is true, the node acts as a lighthouse for others.
     */
    class BootstrapManager {
    public:
        BootstrapManager(bool is_seed);

        // Attempts to find and connect to the network
        void bootstrap();

        // Registers a new peer in the local directory
        void register_peer(const std::string& ip);

        std::vector<std::string> get_known_peers() const;

    private:
        bool is_seed_node_;
        std::vector<std::string> peer_directory_;
        std::unique_ptr<DNSDiscovery> dns_discovery_;
    };
}
