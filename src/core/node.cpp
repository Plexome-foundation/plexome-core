#include "plexome_types.h"
#include "memory_manager.h"
#include <iostream>
#include <memory>

class PlexomeNode {
public:
    PlexomeNode(plexome::NodeRole role) {
        // Initialize memory manager with 4GB VRAM and 8GB RAM limits for testing
        mem_manager_ = std::make_unique<plexome::MemoryManager>(
            4ULL * 1024 * 1024 * 1024, 
            8ULL * 1024 * 1024 * 1024
        );
        identity_.role = role;
    }

    void start() {
        std::cout << "[Plexome] Node " << identity_.id << " starting..." << std::endl;
        
        // Simulate loading a shard
        plexome::ShardID test_shard = {0x01, 0x02};
        mem_manager_->pin_shard(test_shard, 128 * 1024 * 1024); // 128MB shard

        std::cout << "[Plexome] Node is running on port " << plexome::DEFAULT_PORT << std::endl;
    }

private:
    plexome::NodeIdentity identity_ = {"pxm_alpha_01", plexome::NodeRole::Titan, 0, 0, 0};
    std::unique_ptr<plexome::MemoryManager> mem_manager_;
};

int main() {
    PlexomeNode node(plexome::NodeRole::Titan);
    node.start();
    return 0;
}
