/**
 * PLEXOME | Training Controller
 * Manages local LoRA fine-tuning tasks based on ingested hardware manuals.
 */

#include "training_controller.h"
#include <iostream>

namespace plexome {

    void TrainingController::create_tasks_from_knowledge(const std::vector<std::string>& data_blocks) {
        for (const auto& block : data_blocks) {
            // In a real scenario, this would format the block into a JSON training pair
            std::cout << "[Training] Queuing new knowledge block (" << block.size() << " bytes) for LoRA adapter." << std::endl;
            
            // Push to the global task manager as a high-priority training task
            // task_manager_->push_task(TaskType::TRAIN, block);
        }
    }

    bool TrainingController::is_training_active() const {
        return active_tasks_count_ > 0;
    }
}
