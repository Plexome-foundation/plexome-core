#include "plexome_types.h"
#include "memory_manager.h"
#include "shard_storage.h"
#include "network_bus.h"
#include <iostream>
#include <memory>

class PlexomeNode {
public:
    PlexomeNode(plexome::NodeRole role) {
        identity_.role = role;

        // Initialize L1/L2 (Memory)
        mem_manager_ = std::make_unique<plexome::MemoryManager>(4ULL*1024*1024*1024, 8ULL*1024*1024*1024);
        
        // Initialize L3 (Persistence)
        storage_ = std::make_unique<plexome::ShardStorage>("./plexome_data");

        // Initialize Networking (Mock for now)
        network_ = std::make_unique<plexome::MockNetworkBus>();
    }

    void start() {
        std::cout << "[Plexome] Starting Node Identity: " << identity_.id << std::endl;
        
        network_->start(plexome::DEFAULT_PORT);
        network_->broadcast_state("Node online: " + identity_.id);

        // Simulation: Try to save a test shard
        plexome::ShardID test_id = {0xDE, 0xAD, 0xBE, 0xEF};
        std::vector<uint8_t> test_data = {1, 2, 3, 4, 5};
        
        if (!storage_->exists(test_id)) {
            storage_->save_shard(test_id, test_data);
        }

        std::cout << "[Plexome] Running. Press Ctrl+C to stop." << std::endl;
    }

private:
    plexome::NodeIdentity identity_ = {"pxm_arch_01", plexome::NodeRole::Archivist, 0, 0, 0};
    std::unique_ptr<plexome::MemoryManager> mem_manager_;
    std::unique_ptr<plexome::ShardStorage> storage_;
    std::unique_ptr<plexome::NetworkBus> network_;
};

int main() {
    PlexomeNode node(plexome::NodeRole::Archivist);
    node.start();
    return 0;
}
