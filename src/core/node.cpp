/**
 * PLEXOME FOUNDATION | Core Node Entry Point
 * Language: C++20
 * Platform: Windows (Primary) / Linux (Cross-platform)
 */

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

#include "plexome_types.h"
#include "config_loader.h"
#include "stats_collector.h"
#include "cli_interface.h"
#include "web_ui.h"
#include "inference_engine.h"
#include "memory_manager.h"
#include "shard_storage.h"
#include "task_manager.h"
#include "consensus_engine.h"
#include "bootstrap_manager.h"
#include "connection_manager.h"
#include "gossip_service.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>
#include <atomic>

class PlexomeNode {
public:
    PlexomeNode(const std::string& config_path) {
        // 1. Load configuration from file
        config_ = plexome::ConfigLoader::load_file(config_path);

        // 2. Initialize Core Subsystems
        stats_ = std::make_unique<plexome::StatsCollector>();
        cli_ = std::make_unique<plexome::CLIInterface>();
        
        // Storage and Memory (AI-RAID Layer)
        storage_ = std::make_unique<plexome::ShardStorage>(config_.storage_path);
        mem_manager_ = std::make_unique<plexome::MemoryManager>(
            config_.vram_limit_bytes, 
            config_.ram_limit_bytes
        );

        // Network and Swarm Logic
        bootstrap_ = std::make_unique<plexome::BootstrapManager>(config_.is_seed);
        conn_manager_ = std::make_unique<plexome::ConnectionManager>();
        gossip_ = std::make_unique<plexome::GossipService>();
        task_manager_ = std::make_unique<plexome::TaskManager>();
        consensus_ = std::make_unique<plexome::ConsensusEngine>();

        // AI Engine (llama.cpp integration)
        engine_ = plexome::EngineFactory::create_default();
        
        is_running_ = true;
    }

    void start() {
        std::cout << "[Plexome] Initializing Node Identity..." << std::endl;
        
        // Set the active model in stats
        stats_->set_current_model("DeepSeek-Coder-Plexome-v1");

        // A. Startup Bootstrap (DNS Seeds discovery)
        bootstrap_->bootstrap();

        // B. Launch Management Threads
        std::thread cli_thread(&PlexomeNode::run_cli, this);
        std::thread web_thread(&PlexomeNode::run_web_exporter, this);

        // C. Main Operation Loop (Pull Model logic)
        std::cout << "[Plexome] Swarm Node is ACTIVE on port " << config_.port << std::endl;
        
        while (is_running_) {
            process_swarm_logic();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (cli_thread.joinable()) cli_thread.join();
        if (web_thread.joinable()) web_thread.join();
    }

    void stop() {
        is_running_ = false;
    }

private:
    plexome::AppConfig config_;
    std::atomic<bool> is_running_;

    // Core Components
    std::unique_ptr<plexome::StatsCollector> stats_;
    std::unique_ptr<plexome::CLIInterface> cli_;
    std::unique_ptr<plexome::InferenceEngine> engine_;
    std::unique_ptr<plexome::MemoryManager> mem_manager_;
    std::unique_ptr<plexome::ShardStorage> storage_;
    
    // Networking Components
    std::unique_ptr<plexome::BootstrapManager> bootstrap_;
    std::unique_ptr<plexome::ConnectionManager> conn_manager_;
    std::unique_ptr<plexome::GossipService> gossip_;
    std::unique_ptr<plexome::TaskManager> task_manager_;
    std::unique_ptr<plexome::ConsensusEngine> consensus_;

    void run_cli() {
        cli_->run_loop(*stats_);
        stop(); // Shutdown node if CLI exits
    }

    void run_web_exporter() {
        while (is_running_) {
            // Export the dashboard to a local HTML file
            plexome::WebUIExporter::export_dashboard(*stats_, "dashboard.html");
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    void process_swarm_logic() {
        // 1. Check for incoming tasks from the network (simulation)
        auto task = task_manager_->pull_next_task();
        if (task) {
            std::cout << "[Core] Executing coding task: " << task->task_id << std::endl;
            
            // 2. Perform AI Inference
            std::string result = engine_->predict("Analyze code integrity for: " + task->task_id);
            
            // 3. Submit for Majority Vote
            consensus_->submit_result(task->task_id, {1.0f, 0.0f}); // Placeholder for actual tensor output
            
            stats_->report_task_complete();
        }

        // 4. Handle Gossip propagation (Spread the news)
        // gossip_->select_targets(...);
    }
};

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Windows-specific Socket Initialization (Winsock)
    WSADATA wsaData;
    int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsa_result != 0) {
        std::cerr << "[Critical] Winsock initialization failed: " << wsa_result << std::endl;
        return 1;
    }
    std::cout << "[System] Winsock initialized successfully." << std::endl;
#endif

    try {
        // Start the Node using plexome.conf
        PlexomeNode node("plexome.conf");
        node.start();
    } catch (const std::exception& e) {
        std::cerr << "[Fatal] Node crashed: " << e.what() << std::endl;
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
