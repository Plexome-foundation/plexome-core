#include "task_manager.h"
#include <iostream>

namespace plexome {

    TaskManager::TaskManager() {
        std::cout << "[Task] Task Manager initialized. Ready for work orders." << std::endl;
    }

    void TaskManager::push_task(const Task& task) {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(task);
        std::cout << "[Task] New task added to queue: " << task.task_id << std::endl;
    }

    std::optional<Task> TaskManager::pull_next_task() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.empty()) return std::nullopt;

        Task t = queue_.top();
        queue_.pop();
        return t;
    }

    size_t TaskManager::pending_count() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }
}
