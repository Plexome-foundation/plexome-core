#include "stats_collector.h"
#include <sstream>

namespace plexome {

    void StatsCollector::report_task_complete() {
        tasks_done_++;
    }

    void StatsCollector::set_current_model(const std::string& model_name) {
        model_ = model_name;
    }

    SwarmStats StatsCollector::get_snapshot() const {
        return {
            124,    // Mock: 124 nodes active in the swarm
            model_, 
            15,     // Mock: 15 tasks pending
            tasks_done_, 
            8,      // Mock: 8 nodes currently training LoRA
            450.5   // Mock: 450 Mbps throughput
        };
    }

    std::string StatsCollector::to_json() const {
        auto s = get_snapshot();
        std::stringstream ss;
        ss << "{"
           << "\"nodes\":" << s.active_nodes << ","
           << "\"model\":\"" << s.current_model << "\","
           << "\"tasks_pending\":" << s.pending_tasks
           << "}";
        return ss.str();
    }
}
