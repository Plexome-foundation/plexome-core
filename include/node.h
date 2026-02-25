#pragma once
#include "plexome_types.h"
#include <memory>
#include <atomic>

namespace plexome {
    class ConnectionManager;
    class KnowledgeManager;
    class InferenceEngine;

    class Node {
    public:
        explicit Node(const AppConfig& config);
        ~Node();
        void init();
        void run();
        void stop();
    private:
        void run_cli();
        void process_core_logic();
        AppConfig config_;
        std::atomic<bool> is_running_;
        std::unique_ptr<InferenceEngine> engine_;
        std::unique_ptr<ConnectionManager> conn_manager_;
        std::unique_ptr<KnowledgeManager> knowledge_;
    };
}
