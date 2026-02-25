#include "plexome_types.h"
#include "inference_engine.h"
#include "stats_collector.h"
#include <iostream>

class PlexomeNode {
public:
    PlexomeNode() {
        engine_ = plexome::EngineFactory::create_default();
        stats_ = std::make_unique<plexome::StatsCollector>();
    }

    void start() {
        std::cout << "[Plexome] Node starting with Programming AI Core..." << std::endl;
        
        // Use a lightweight coding model for the start (e.g., DeepSeek-Coder-1.3B)
        if (engine_->load_model("./models/coder-1.3b.gguf")) {
            stats_->set_current_model("DeepSeek-Coder-Plexome-v1");
        }

        // Test inference
        std::string code_task = "Write a C++ function to shard a file into 4MB chunks.";
        std::string result = engine_->predict(code_task);
        
        std::cout << "[Plexome] Initial self-test result: " << result << std::endl;
    }

private:
    std::unique_ptr<plexome::InferenceEngine> engine_;
    std::unique_ptr<plexome::StatsCollector> stats_;
};
