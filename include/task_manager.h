#pragma once
#include "plexome_types.h"
#include <vector>
#include <string>
#include <mutex>
#include <optional>

namespace plexome {

    struct Task {
        std::string task_id;
        std::string payload;
        bool is_completed = false;
    };

    class TaskManager {
    public:
        TaskManager() = default;

        // Adds a new computation task to the swarm queue
        void push_task(const std::string& id, const std::string& data);

        // Safely retrieves the next available task
        std::optional<Task> pull_next_task();

        size_t get_queue_size();

    private:
        std::vector<Task> queue_;
        std::mutex mtx_;
    };
}
