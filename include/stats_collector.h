#pragma once
#include "plexome_types.h"
#include <string>
#include <map>
#include <vector>

namespace plexome {

    struct SwarmStats {
        uint32_t active_nodes;
        std::string current_model;
        uint32_t pending_tasks;
        uint32_t completed_tasks;
        uint32_t training_nodes_count;
        double network_throughput_mbps;
    };

    class StatsCollector {
    public:
        // Updates local and global metrics
        void report_task_complete();
        void set_current_model(const std::string& model_name);
        
        // Generates a snapshot for UI
        SwarmStats get_snapshot() const;

        // Returns JSON string for Web Interface
        std::string to_json() const;

    private:
        uint32_t tasks_done_ = 0;
        std::string model_ = "None";
    };
}
