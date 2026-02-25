#pragma once
#include "plexome_types.h"
#include <string>
#include <vector>
#include <queue>
#include <mutex>

namespace plexome {

    /**
     * Represents a computation unit (Inference or Training step)
     */
    struct Task {
        std::string task_id;
        ShardID target_shard;
        std::vector<float> input_data;
        uint64_t priority;
        uint64_t timestamp;
    };

    /**
     * Handles the Pull-model queue for the node.
     * Nodes pull tasks based on their available VRAM and Role.
     */
    class TaskManager {
    public:
        TaskManager();

        // Add a task to the local queue (pulled from network)
        void push_task(const Task& task);

        // Fetch the next high-priority task for processing
        std::optional<Task> pull_next_task();

        size_t pending_count() const;

    private:
        // Priority queue to handle tasks by urgency
        struct TaskComparator {
            bool operator()(const Task& a, const Task& b) {
                return a.priority < b.priority;
            }
        };

        std::priority_queue<Task, std::vector<Task>, TaskComparator> queue_;
        mutable std::mutex mtx_;
    };
}
