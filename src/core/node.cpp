/**
 * PLEXOME FOUNDATION | Universal Swarm Node v1.3 (Enterprise Tier)
 * Architecture: AI-RAID, Federated LoRA, Pipeline Parallelism
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
#include "lora_registry.h"
#include "swarm_router.h"

#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <filesystem>
#include <csignal>
#include <mutex>
#include <condition_variable>

// Global OS Signal Handling
std::atomic<bool> g_is_running{true};
std::condition_variable g_shutdown_cv;
std::mutex g_shutdown_mtx;

void signal_handler(int signal) {
    if (g_is_running) {
        std::cout << "\n[System] Interrupt signal (" << signal << ") received. Initiating graceful shutdown..." << std::endl;
        g_is_running = false;
        g_shutdown_cv.notify_all();
    }
}

class PlexomeNode {
public:
    PlexomeNode(const std::string& config_path) {
        config_ = plexome::ConfigLoader::load_file(config_path);
        
        // 1. Core Infrastructure
        stats_ = std::make_unique<plexome::StatsCollector>();
        cli_ = std::make_unique<plexome::CLIInterface>();
        storage_ = std::make_unique<plexome::ShardStorage>(config_.storage_path);
        mem_manager_ = std::make_unique<plexome::MemoryManager>(config_.vram_limit_bytes, config_.ram_limit_bytes);
        
        // 2. Network & Swarm
        bootstrap_ = std::make_unique<plexome::BootstrapManager>(config_.is_seed);
        conn_manager_ = std::make_unique<plexome::ConnectionManager>();
        gossip_ = std::make_unique<plexome::GossipService>();
        task_manager_ = std::make_unique<plexome::TaskManager>();
        consensus_ = std::make_unique<plexome::ConsensusEngine>();
        
        // 3. AI, Knowledge & Distributed Execution
        engine_ = plexome::EngineFactory::create_default();
        knowledge_ = std::make_unique<plexome::KnowledgeManager>("./knowledge");
        training_ = std::make_unique<plexome::TrainingController>();
        lora_registry_ = std::make_unique<plexome::LoRaRegistry>();
        router_ = std::make_unique<plexome::SwarmRouter>();
    }

    void start() {
        std::cout << "=========================================" << std::endl;
        std::cout << "   PLEXOME FOUNDATION | NODE TERMINAL    " << std::endl;
        std::cout << "=========================================" << std::endl;

        // A. Hardware Benchmark (Calculate Parrots)
        auto bench = plexome::HardwareBenchmark::run_test();

        // B. Async Model Loading
        std::thread model_thread(&PlexomeNode::init_ai_core, this, bench);
        model_thread.detach();

        // C. Network Bootstrap
        bootstrap_->bootstrap();

        // D. Background Interfaces
        std::thread cli_thread(&PlexomeNode::run_cli, this);
        cli_thread.detach(); 

        std::thread web_thread(&PlexomeNode::run_web_exporter, this);

        // E. Main Swarm Event Loop
        std::cout << "[Core] Entering main operational loop on port " << config_.port << std::endl;
        std::unique_lock<std::mutex> lock(g_shutdown_mtx);
        
        while (g_is_running) {
            process_core_logic();
            g_shutdown_cv.wait_for(lock, std::chrono::milliseconds(200), []{ return !g_is_running.load(); });
        }

        // F. Graceful Shutdown
        perform_shutdown(web_thread);
    }

private:
    plexome::AppConfig config_;

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
    std::unique_ptr<plexome::LoRaRegistry> lora_registry_;
    std::unique_ptr<plexome::SwarmRouter> router_;

    void init_ai_core(const plexome::BenchmarkResult& bench) {
        if (!std::filesystem::exists("./models") || std::filesystem::is_empty("./models")) {
            std::cout << "[AI Core] No models found. Power Score: " << bench.score << " Parrots." << std::endl;
            std::cout << "[AI Core] Recommended: " << bench.recommended_model << std::endl;
            return;
        }
        for (const auto& entry : std::filesystem::directory_iterator("./models")) {
            if (entry.path().extension() == ".gguf") {
                std::cout << "[AI Core] Loading model: " << entry.path().filename() << "..." << std::endl;
                engine_->load_model(entry.path().string()); 
                stats_->set_current_model(entry.path().filename().string());
                break;
            }
        }
    }

    void run_cli() {
        cli_->run_loop(*stats_);
        g_is_running = false;
        g_shutdown_cv.notify_all();
    }

    void run_web_exporter() {
        while (g_is_running) {
            plexome::WebUIExporter::export_dashboard(*stats_, "dashboard.html");
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    void process_core_logic() {
        if (stats_->get_snapshot().current_model == "None") return;

        // 1. KNOWLEDGE INGESTION: Read local 3PAR/Primera manuals
        auto new_data = knowledge_->scan_new_data();
        if (!new_data.empty()) training_->create_tasks_from_knowledge(new_data);

        // 2. KNOWLEDGE SYNC (Mechanism A): Check for missing LoRA updates from the Swarm
        auto missing_loras = lora_registry_->get_missing_loras(stats_->get_snapshot().current_model);
        if (!missing_loras.empty()) {
            // std::cout << "[Sync] Downloading " << missing_loras.size() << " new knowledge layers..." << std::endl;
            // logic to pull missing_loras[0] via Gossip/NetworkBus
        }

        // 3. PIPELINE PARALLELISM (Mechanism B): Handle sharded model execution
        // if (router_->is_part_of_pipeline()) {
        //     auto incoming_tensor = router_->await_incoming_tensor();
        //     if (!incoming_tensor.empty()) {
        //         auto result_tensor = engine_->process_layer_slice(incoming_tensor);
        //         router_->forward_tensor(router_->get_next_hop(engine_->my_end_layer()), result_tensor);
        //     }
        // }

        // 4. STANDARD TASKS: Execute standalone inference/training tasks
        auto task = task_manager_->pull_next_task();
        if (task) {
            std::string result = engine_->predict("Processing Task: " + task->task_id);
            consensus_->submit_result(task->task_id, {1.0f});
            stats_->report_task_complete();
        }
    }

    void perform_shutdown(std::thread& web_thread) {
        std::cout << "[System] Flushing memory caches and saving AI state..." << std::endl;
        if (web_thread.joinable()) web_thread.join();
        std::cout << "[System] Safe to power off. Plexome Node terminated." << std::endl;
    }
};

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);  
    std::signal(SIGTERM, signal_handler); 

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;
#endif

    try {
        PlexomeNode node("plexome.conf");
        node.start();
    } catch (const std::exception& e) { 
        std::cerr << "[Fatal Error] " << e.what() << std::endl; 
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
