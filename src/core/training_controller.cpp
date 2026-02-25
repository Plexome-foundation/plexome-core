#include "training_controller.h"
#include <iostream>

namespace plexome {

    void TrainingController::create_tasks_from_knowledge(const std::vector<KnowledgePacket>& data_packets) {
        for (const auto& packet : data_packets) {
            std::cout << "[Training] Queuing new knowledge block from " 
                      << packet.source_name << " (" << packet.data.size() << " bytes) for LoRA adapter." << std::endl;
            
            // In a real scenario, this would push a properly formatted Task to the task_manager
            Task new_task;
            new_task.task_id = "LORA_TUNE_" + packet.source_name;
            new_task.payload = packet.data;
            
            task_queue_.push(new_task);
            active_tasks_count_++;
        }
    }

    bool TrainingController::is_training_active() const {
        return active_tasks_count_ > 0;
    }
}
