#include "task_manager.h"

namespace plexome {

    void TaskManager::push_task(const std::string& id, const std::string& data) {
        std::lock_guard<std::mutex> lock(mtx_);
        Task t;
        t.task_id = id;
        t.payload = data;
        t.is_completed = false;
        queue_.push_back(t);
    }

    std::optional<Task> TaskManager::pull_next_task() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        
        // Take the first task and remove it from the vector
        Task t = queue_.front();
        queue_.erase(queue_.begin());
        return t;
    }

    size_t TaskManager::get_queue_size() {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

}
