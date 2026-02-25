#include "plexome_types.h"
#include "bootstrap_manager.h"
#include "connection_manager.h"
#include "integrity_checker.h"
#include "gossip_service.h"
#include <iostream>
#include <memory>

class PlexomeNode {
public:
    PlexomeNode(plexome::NodeRole role, bool is_seed) {
        bootstrap_ = std::make_unique<plexome::BootstrapManager>(is_seed);
        conn_manager_ = std::make_unique<plexome::ConnectionManager>();
        gossip_ = std::make_unique<plexome::GossipService>();
        identity_.role = role;
    }

    void start() {
        bootstrap_->bootstrap();
        
        // Simulation: Recieve shard data and verify integrity
        std::vector<uint8_t> shard_data = {0xAA, 0xBB, 0xCC};
        plexome::ShardID expected_id = {0xAA}; // Mock expected ID
        
        if (plexome::IntegrityChecker::verify(shard_data, expected_id)) {
            std::cout << "[Plexome] Shard integrity verified. Adding to L2 Cache." << std::endl;
        } else {
            std::cerr << "[Plexome] Data corruption detected!" << std::endl;
        }

        // Simulation: Receive a Gossip message
        std::string msg_id = "broadcast_001";
        if (gossip_->process_message(msg_id)) {
            std::cout << "[Gossip] New network update received: " << msg_id << std::endl;
        }
    }

private:
    plexome::NodeIdentity identity_ = {"pxm_main_01", plexome::NodeRole::Titan, 0, 0, 0};
    std::unique_ptr<plexome::BootstrapManager> bootstrap_;
    std::unique_ptr<plexome::ConnectionManager> conn_manager_;
    std::unique_ptr<plexome::GossipService> gossip_;
};

int main(int argc, char* argv[]) {
    bool run_as_seed = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--seed") run_as_seed = true;
    }

    PlexomeNode node(plexome::NodeRole::Titan, run_as_seed);
    node.start();
    return 0;
}
