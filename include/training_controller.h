#pragma once
#include "plexome_types.h"
#include "knowledge_manager.h"
#include <queue>

namespace plexome {

    /**
     * Manages the Federated LoRA training process.
     */
    class TrainingController {
    public:
        // Converts raw text into structured training tasks for the swarm
        void create_tasks_from_knowledge(const std::vector<KnowledgePacket>& packets);

        // Submits a finished LoRA delta to the swarm
        void broadcast_lora_delta(const std::vector<uint8_t>& delta_weights);

    private:
        // Internal queue of shards waiting to be trained
        std::queue<Task> training_queue_;
    };
}
