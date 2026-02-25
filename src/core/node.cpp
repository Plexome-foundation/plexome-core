/**
 * PLEXOME FOUNDATION | Universal Swarm Node v1.0
 * Architecture: AI-RAID & Federated Learning
 * Platform: Windows/Linux Cross-Platform
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
#include "knowledge_manager.h"
#include "training_controller.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include <string>

class PlexomeNode {
public:
    PlexomeNode(const std::string& config_path) {
        // 1. Load Settings
        config_ = plexome::ConfigLoader::load_file(config_path);

        // 2. Core Infrastructure
        stats_ = std::make_unique<plexome::StatsCollector>();
        cli_ = std::make_unique<plexome::CLIInterface>();
        storage_ = std::make_unique<plexome::ShardStorage>(config_.storage_path);
        mem_manager_ = std::make_unique<plexome::MemoryManager>(config_.vram_limit_bytes, config_.ram_limit_bytes);

        // 3. Network & Swarm
        bootstrap_ = std::make_unique<plexome::BootstrapManager>(config_.is_seed);
        conn_manager_ = std::make_unique<plexome::ConnectionManager>();
        gossip_ = std::make_unique<plexome::GossipService>();
        task_manager_ = std::make_unique<plexome::TaskManager>();
        consensus_ = std::make_unique<plexome::ConsensusEngine>();

        // 4. AI & Knowledge (The "Brain")
        engine_ = plexome::EngineFactory::create_default();
        knowledge_ = std::make_unique<plexome::KnowledgeManager>("./knowledge");
        training_ = std::make_unique<plexome::TrainingController>();

        is_running_ = true;
    }

    void start() {
        std::cout << "[Plexome] Node Booting... Mode: " << (config_.is_seed ? "SEED/BASE" : "NORMAL") << std::endl;
        
        // Load initial model for programming & storage expertise
        if (engine_->load_model("./models/coder-1.3b.gguf")) {
            stats_->set_current_model("Plexome-DeepSeek-HPE-v1");
        }

        bootstrap_->bootstrap();

        // Threads: CLI Management and Web Dashboard Export
        std::thread cli_thread(&PlexomeNode::run_cli, this);
        std::thread web_thread(&PlexomeNode::run_web_exporter, this);

        // Main Execution Loop
        while (is_running_) {
            process_core_logic();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        if (cli_thread.joinable()) cli_thread.join();
        if (web_thread.joinable()) web_thread.join();
    }

    void stop() { is_running_ = false; }

private:
    plexome::AppConfig config_;
    std::atomic<bool> is_running_;

    // Subsystems
    std::unique_ptr<plexome::StatsCollector> stats_;
    std::unique_ptr<plexome::CLIInterface> cli_;
    std::unique_ptr<plexome::InferenceEngine> engine_;
    std::unique_ptr<plexome::MemoryManager> mem_manager_;
    std::unique_ptr<plexome::ShardStorage> storage_;
    std::unique_ptr<plexome::KnowledgeManager> knowledge_;
    std::unique_ptr<plexome::TrainingController> training_;
    
    // Network
    std::unique_ptr<plexome::BootstrapManager> bootstrap_;
    std::unique_ptr<plexome::ConnectionManager> conn_manager_;
    std::unique_ptr<plexome::GossipService> gossip_;
    std::unique_ptr<plexome::TaskManager> task_manager_;
    std::unique_ptr<plexome::ConsensusEngine> consensus_;

    void run_cli() {
        cli_->run_loop(*stats_);
        stop();
    }

    void run_web_exporter() {
        while (is_running_) {
            plexome::WebUIExporter::export_dashboard(*stats_, "dashboard.html");
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    void process_core_logic() {
        // 1. KNOWLEDGE SCAN: Look for new 3PAR/Alletra manuals
        auto new_data = knowledge_->scan_new_data();
        if (!new_data.empty()) {
            std::cout << "[Knowledge] Injecting " << new_data.size() << " blocks into training queue." << std::endl;
            training_->create_tasks_from_knowledge(new_data);
        }

        // 2. SWARM TASKS: Pull and execute computation
        auto task = task_manager_->pull_next_task();
        if (task) {
            std::string result = engine_->predict("Technical Query: " + task->task_id);
            consensus_->submit_result(task->task_id, {1.0f}); // Mock tensor result
            stats_->report_task_complete();
        }
    }
};

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;
#endif

    try {
        PlexomeNode node("plexome.conf");
        node.start();
    } catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << std::endl;
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
