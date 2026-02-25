#include "plexome_types.h"
#include "config_loader.h"
#include "stats_collector.h"
#include "cli_interface.h"
#include "web_ui.h"
#include <iostream>
#include <thread>
#include <chrono>

class PlexomeNode {
public:
    PlexomeNode(const std::string& config_file) {
        // Step 1: Load external configuration
        config_ = plexome::ConfigLoader::load_file(config_file);
        
        stats_ = std::make_unique<plexome::StatsCollector>();
        cli_ = std::make_unique<plexome::CLIInterface>();
    }

    void start() {
        std::cout << "[Plexome] Starting Swarm Node on port " << config_.port << std::endl;
        stats_->set_current_model("Llama-3-8B-Plexome-v1");

        // Thread A: Background CLI
        std::thread cli_thread([this]() {
            cli_->run_loop(*stats_);
        });

        // Thread B: Web UI Exporter (updates index.html every 2 seconds)
        std::thread web_thread([this]() {
            while (true) {
                plexome::WebUIExporter::export_dashboard(*stats_, "dashboard.html");
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });

        // Thread C: Main Simulation Logic
        while (true) {
            // Simulate task processing for the demo
            std::this_thread::sleep_for(std::chrono::seconds(5));
            stats_->report_task_complete();
        }

        if (cli_thread.joinable()) cli_thread.join();
        if (web_thread.joinable()) web_thread.join();
    }

private:
    plexome::AppConfig config_;
    std::unique_ptr<plexome::StatsCollector> stats_;
    std::unique_ptr<plexome::CLIInterface> cli_;
};

int main(int argc, char* argv[]) {
    PlexomeNode node("plexome.conf");
    node.start();
    return 0;
}
