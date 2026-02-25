#include "plexome_types.h"
#include "memory_manager.h"
#include "shard_storage.h"
#include "network_bus.h"
#include "task_manager.h"
#include "consensus_engine.h"
#include <iostream>
#include <memory>

class PlexomeNode {
public:
    PlexomeNode(plexome::NodeRole role) {
        identity_.role = role;
        
        // Storage and Memory Layers
        mem_manager_ = std::make_unique<plexome::MemoryManager>(4ULL*1024*1024*1024, 8ULL*1024*1024*1024);
        storage_ = std::make_unique<plexome::ShardStorage>("./plexome_data");
        
        // Swarm Logic Layers
        task_manager_ = std::make_unique<plexome::TaskManager>();
        consensus_ = std::make_unique<plexome::ConsensusEngine>();
        network_ = std::make_unique<plexome::MockNetworkBus>();
    }

    void start() {
        std::cout << "[Plexome] Node " << identity_.id << " is ONLINE." << std::endl;
        network_->start(plexome::DEFAULT_PORT);

        // Simulation: Recieve a task from network
        plexome::Task t = {"task_001", {0x01}, {0.5f, 0.2f}, 10, 12345678};
        task_manager_->push_task(t);

        // Pull and process
        auto next_task = task_manager_->pull_next_task();
        if (next_task) {
            std::cout << "[Plexome] Processing task: " << next_task->task_id << std::endl;
            // Here we would call llama.cpp logic
            consensus_->submit_result(next_task->task_id, {0.99f, 0.01f});
        }
    }

private:
    plexome::NodeIdentity identity_ = {"pxm_titan_01", plexome::NodeRole::Titan, 0, 0, 0};
    std::unique_ptr<plexome::MemoryManager> mem_manager_;
    std::unique_ptr<plexome::ShardStorage> storage_;
    std::unique_ptr<plexome::NetworkBus> network_;
    std::unique_ptr<plexome::TaskManager> task_manager_;
    std::unique_ptr<plexome::ConsensusEngine> consensus_;
};

int main() {
    PlexomeNode node(plexome::NodeRole::Titan);
    node.start();
    return 0;
}
