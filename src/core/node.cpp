/**
 * PLEXOME FOUNDATION | Universal Swarm Node v1.1
 * Includes: Hardware Benchmarking & Smart Model Suggestion
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
#include "benchmark.h"
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
#include <atomic>
#include <string>
#include <filesystem>

class PlexomeNode {
public:
    PlexomeNode(const std::string& config_path) {
        config_ = plexome::ConfigLoader::load_file(config_path);
        stats_ = std::make_unique<plexome::StatsCollector>();
        cli_ = std::make_unique<plexome::CLIInterface>();
        storage_ = std::make_unique<plexome::ShardStorage>(config_.storage_path);
        mem_manager_ = std::make_unique<plexome::MemoryManager>(config_.vram_limit_bytes, config_.ram_limit_bytes);
        bootstrap_ = std::make_unique<plexome::BootstrapManager>(config_.is_seed);
        conn_manager_ = std::make_unique<plexome::ConnectionManager>();
        gossip_ = std::make_unique<plexome::GossipService>();
        task_manager_ = std::make_unique<plexome::TaskManager>();
        consensus_ = std::make_unique<plexome::ConsensusEngine>();
        engine_ = plexome::EngineFactory::create_default();
        knowledge_ = std::make_unique<plexome::KnowledgeManager>("./knowledge");
        training_ = std::make_unique<plexome::TrainingController>();

        is_running_ = true;
    }

    void start() {
        std::cout << "--- PLEXOME STARTUP SEQUENCE ---" << std::endl;

        // 1. HARDWARE TEST
        auto bench = plexome::HardwareBenchmark::run_test();
        check_for_model(bench);

        // 2. BOOTSTRAP
        bootstrap_->bootstrap();

        // 3. THREADS
        std::thread cli_thread(&PlexomeNode::run_cli, this);
        std::thread web_thread(&PlexomeNode::run_web_exporter, this);

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
    std::unique_ptr<plexome::StatsCollector> stats_;
    std::unique_ptr<plexome::CLIInterface> cli_;
    std::unique_ptr<plexome::InferenceEngine> engine_;
    std::unique_ptr<plexome::MemoryManager> mem_manager_;
    std::unique_ptr<plexome::ShardStorage> storage_;
    std::unique_ptr<plexome::KnowledgeManager> knowledge_;
    std::unique_ptr<plexome::TrainingController> training_;
    std::unique_ptr<plexome::BootstrapManager> bootstrap_;
    std::unique_ptr<plexome::ConnectionManager> conn_manager_;
    std::unique_ptr<plexome::GossipService> gossip_;
    std::unique_ptr<plexome::TaskManager> task_manager_;
    std::unique_ptr<plexome::ConsensusEngine> consensus_;

    void check_for_model(const plexome::BenchmarkResult& bench) {
        if (!std::filesystem::exists("./models") || std::filesystem::is_empty("./models")) {
            std::cout << "\n[ALERT] No AI models found in /models/ folder!" << std::endl;
            std::cout << "[SYSTEM] Your power score: " << bench.score << " Parrots." << std::endl;
            std::cout << "[SYSTEM] Recommended for you: " << bench.recommended_model << std::endl;
            std::cout << "[SYSTEM] Please run 'scripts/download_models.ps1' or download manually.\n" << std::endl;
        } else {
            // Try to load the first available model
            for (const auto& entry : std::filesystem::directory_iterator("./models")) {
                if (entry.path().extension() == ".gguf") {
                    engine_->load_model(entry.path().string());
                    stats_->set_current_model(entry.path().filename().string());
                    break;
                }
            }
        }
    }

    void run_cli() { cli_->run_loop(*stats_); stop(); }
    void run_web_exporter() {
        while (is_running_) {
            plexome::WebUIExporter::export_dashboard(*stats_, "dashboard.html");
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    void process_core_logic() {
        auto new_data = knowledge_->scan_new_data();
        if (!new_data.empty()) training_->create_tasks_from_knowledge(new_data);

        auto task = task_manager_->pull_next_task();
        if (task) {
            engine_->predict("Task: " + task->task_id);
            consensus_->submit_result(task->task_id, {1.0f});
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
    } catch (const std::exception& e) { std::cerr << "[Fatal] " << e.what() << std::endl; }
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
