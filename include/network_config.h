#pragma once
#include <vector>
#include <string>

namespace plexome {
    // DNS Seeds for initial swarm discovery
    const std::vector<std::string> DNS_SEEDS = {
        "seed1.plexome.ai",
        "seed2.plexome.ai",
        "bootstrap.plexome.ai"
    };

    // Protocol constants
    const uint16_t P2P_PORT = 7539;
    const uint32_t MAX_PEERS_PER_NODE = 50;
    const uint32_t SEED_REFRESH_INTERVAL_SEC = 3600;
}
