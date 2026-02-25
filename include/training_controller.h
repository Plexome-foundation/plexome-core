#pragma once
#include "plexome_types.h"
#include "task_manager.h" // КРИТИЧНО: Добавлено для распознавания Task
#include <vector>
#include <string>
#include <queue>

namespace plexome {

    class TrainingController {
    public:
        TrainingController() = default;

        // Processes ingested manuals and converts them into training tasks
        void create_tasks_from_knowledge(const std::vector<KnowledgePacket>& data_packets);

        // Checks if the node is currently fine-tuning a local LoRA model
        bool is_training_active() const;

    private:
        std::queue<Task> task_queue_;
        size_t active_tasks_count_ = 0;
    };
}
