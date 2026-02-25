#include "plexome_types.h"
#include "stats_collector.h"
#include "cli_interface.h"
#include "web_api.h"
#include <iostream>
#include <thread>

class PlexomeNode {
public:
    PlexomeNode() {
        stats_ = std::make_unique<plexome::StatsCollector>();
        cli_ = std::make_unique<plexome::CLIInterface>();
    }

    void start() {
        std::cout << "[Plexome] Starting Node with Management Interfaces..." << std::endl;
        
        stats_->set_current_model("Llama-3-70B-Plexome-v1");

        // Start CLI in a background thread
        std::thread cli_thread([this]() {
            cli_->run_loop(*stats_);
        });

        // Main operation loop
        while (true) {
            // Here the node does its background work: 
            // syncing shards, processing tasks, etc.
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (cli_thread.joinable()) cli_thread.join();
    }

private:
    std::unique_ptr<plexome::StatsCollector> stats_;
    std::unique_ptr<plexome::CLIInterface> cli_;
};

int main() {
    PlexomeNode node;
    node.start();
    return 0;
}
