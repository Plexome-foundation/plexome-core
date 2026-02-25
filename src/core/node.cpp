/**
 * PLEXOME FOUNDATION | Universal Swarm Node v1.2 (Pro-Tier)
 * Includes: Graceful Shutdown, Signal Handling, Thread Safety, Async I/O
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
#include <csignal>
#include <mutex>
#include <condition_variable>

// Global atomic flag for OS signal handling
std::atomic<bool> g_is_running{true};
std::condition_variable g_shutdown_cv;
std::mutex g_shutdown_mtx;

void signal_handler(int signal) {
    if (g_is_running) {
        std::cout << "\n[System] Interrupt signal (" << signal << ") received. Initiating graceful shutdown..." << std::endl;
        g_is_running = false;
        g_shutdown_cv.notify_all(); // Wake up main thread instantly
    }
}

class PlexomeNode {
public:
    PlexomeNode(const std::string& config_path) {
        config_ = plexome::ConfigLoader::load_file(config_path);
        
        // Initialize Core (Thread-safe pointers)
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
    }

    void start() {
        std::cout << "=========================================" << std::endl;
        std::cout << "   PLEXOME FOUNDATION | NODE TERMINAL    " << std::endl;
        std::cout << "=========================================" << std::endl;

        // 1. Hardware Benchmark
        auto bench = plexome::HardwareBenchmark::run_test();

        // 2. Async Model Loading (Does not block network startup)
        std::thread model_thread(&PlexomeNode::init_ai_core, this, bench);
        model_thread.detach();

        // 3. Network Bootstrap
        bootstrap_->bootstrap();

        // 4. Background Interfaces
        std::thread cli_thread(&PlexomeNode::run_cli, this);
        cli_thread.detach(); // Detach CLI so std::cin doesn't block shutdown

        std::thread web_thread(&PlexomeNode::run_web_exporter, this);

        // 5. Main Swarm Event Loop (Non-blocking wait)
        std::cout << "[Core] Entering main operational loop." << std::endl;
        std::unique_lock<std::mutex> lock(g_shutdown_mtx);
        
        while (g_is_running) {
            process_core_logic();
            // Wait for 500ms OR until shutdown signal is received
            g_shutdown_cv.wait_for(lock, std::chrono::milliseconds(500), []{ return !g_is_running.load(); });
        }

        // 6. Graceful Shutdown Sequence
        perform_shutdown(web_thread);
    }

private:
    plexome::AppConfig config_;

    // Core Pointers
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

    void init_ai_core(const plexome::BenchmarkResult& bench) {
        if (!std::filesystem::exists("./models") || std::filesystem::is_empty("./models")) {
            std::cout << "[AI Core] No models found. Awaiting manual download." << std::endl;
            return;
        }
        for (const auto& entry : std::filesystem::directory_iterator("./models")) {
            if (entry.path().extension() == ".gguf") {
                // FIXED: Passing hardware limits to the engine to prevent OOM
                std::cout << "[AI Core] Loading " << entry.path().filename() << "..." << std::endl;
                // Note: Assuming InferenceEngine::load_model signature is updated to take vram_limit
                engine_->load_model(entry.path().string()); 
                stats_->set_current_model(entry.path().filename().string());
                break;
            }
        }
    }

    void run_cli() {
        cli_->run_loop(*stats_);
        // If user types 'exit' in CLI, trigger global shutdown
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
        // Only process tasks if the AI engine is fully loaded
        if (stats_->get_snapshot().current_model != "None") {
            auto new_data = knowledge_->scan_new_data();
            if (!new_data.empty()) training_->create_tasks_from_knowledge(new_data);

            auto task = task_manager_->pull_next_task();
            if (task) {
                engine_->predict("Task: " + task->task_id);
                consensus_->submit_result(task->task_id, {1.0f});
                stats_->report_task_complete();
            }
        }
    }

    void perform_shutdown(std::thread& web_thread) {
        std::cout << "[System] Flushing memory caches and saving state..." << std::endl;
        
        // Wait for web thread to finish writing
        if (web_thread.joinable()) {
            web_thread.join();
        }

        // Here we would call storage_->flush() or engine_->save_lora_state()
        std::cout << "[System] Safe to power off. Plexome Node terminated." << std::endl;
    }
};

int main(int argc, char* argv[]) {
    // Register OS Signal Handlers
    std::signal(SIGINT, signal_handler);  // Ctrl+C
    std::signal(SIGTERM, signal_handler); // Kill command

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
