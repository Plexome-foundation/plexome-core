/**
 * Plexome Core v2.0 - Orchestrator (Host)
 * Author: Georgii
 * * Main entry point for the decentralized node. Coordinates 8 specialized 
 * modules to provide secure, accelerated AI inference.
 */

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <map>

// Plexome Platform & Types
#include "pxm_platform.h"
#include "module_interface.h"
#include "plexome_types.h"

/**
 * Cross-platform loader for dynamic libraries (.dll / .so).
 */
class PxmModuleLoader {
public:
    static void* load(const std::string& name) {
        std::string full_path;
#ifdef PXM_PLATFORM_WINDOWS
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::filesystem::path exe_dir = std::filesystem::path(buffer).parent_path();
        full_path = (exe_dir / (name + ".dll")).string();
        
        void* handle = LoadLibraryA(full_path.c_str());
        if (!handle) {
            std::cerr << "[Host] LNK Error: " << GetLastError() << " for " << name << std::endl;
        }
        return handle;
#else
        full_path = "./" + name + ".so";
        return dlopen(full_path.c_str(), RTLD_NOW);
#endif
    }

    static void* get_proc(void* handle, const std::string& func_name) {
        if (!handle) return nullptr;
#ifdef PXM_PLATFORM_WINDOWS
        return (void*)GetProcAddress((HMODULE)handle, func_name.c_str());
#else
        return dlsym(handle, func_name.c_str());
#endif
    }

    static void unload(void* handle) {
        if (!handle) return;
#ifdef PXM_PLATFORM_WINDOWS
        FreeLibrary((HMODULE)handle);
#else
        dlclose(handle);
#endif
    }
};

int main(int argc, char* argv[]) {
    std::cout << "--- Plexome Core v2.0 Orchestrator ---" << std::endl;
    std::cout << "[Host] Initializing system swarm..." << std::endl;

    // 1. Initial Configuration
    PxmConfig config;
    config.is_seed = false;
    config.network_port = 4242;
    config.tier = PerformanceTier::POTATO; // Default until benchmarked
    config.model_path = "models/phi-3-mini-4k.gguf"; 
    config.work_dir = "./";

    // 2. Execution Sequence (Order is critical for Tier assignment)
    std::vector<std::string> module_names = { 
        "pxm_bench", "pxm_ai", "pxm_network", "pxm_topology", 
        "pxm_sharder", "pxm_updater", "pxm_security", "pxm_dashboard" 
    };
    
    std::map<std::string, void*> loaded_modules;

    // 3. Sequential Initialization
    for (const auto& name : module_names) {
        std::cout << "[Host] Loading " << name << "..." << std::endl;
        void* handle = PxmModuleLoader::load(name);
        if (!handle) continue;

        // Run Benchmark first to set the Tier for other modules
        if (name == "pxm_bench") {
            typedef PerformanceTier (*PXM_RUN_BENCH_FUNC)();
            auto run_bench = (PXM_RUN_BENCH_FUNC)PxmModuleLoader::get_proc(handle, "pxm_run_benchmark");
            if (run_bench) {
                config.tier = run_bench(); 
                std::cout << "  >> System Tier set to: " << (int)config.tier << "" << std::endl;
            }
        }

        // Initialize Module
        auto init_func = (PXM_INIT_FUNC)PxmModuleLoader::get_proc(handle, "pxm_init");
        if (init_func) {
            if (init_func(&config) == PxmStatus::OK) {
                loaded_modules[name] = handle;
            } else {
                std::cerr << "  >> FAILED to initialize " << name << std::endl;
                PxmModuleLoader::unload(handle);
            }
        }
    }

    std::cout << "---------------------------------------" << std::endl;
    std::cout << "[Host] " << loaded_modules.size() << " modules linked. Swarm node ready." << std::endl;

    // 4. Interactive Command Loop (Plexome REPL)
    if (loaded_modules.count("pxm_ai")) {
        std::cout << "\n[Plexome] Georgii, the system is online. Type 'exit' to stop." << std::endl;
        
        typedef const char* (*PXM_GENERATE_FUNC)(const char*);
        auto generate = (PXM_GENERATE_FUNC)PxmModuleLoader::get_proc(loaded_modules["pxm_ai"], "pxm_generate");

        std::string user_input;
        while (true) {
            std::cout << "\n>>> ";
            if (!std::getline(std::cin, user_input) || user_input == "exit") break;
            if (user_input.empty()) continue;

            if (generate) {
                // In tomorrow's update, this will stream tokens from GPU
                std::cout << "[Plexome]: " << generate(user_input.c_str()) << std::endl;
            }
        }
    }

    // 5. Graceful Shutdown
    std::cout << "\n[Host] Shutting down modules..." << std::endl;
    for (auto const& [name, handle] : loaded_modules) {
        auto shutdown_func = (PXM_SHUTDOWN_FUNC)PxmModuleLoader::get_proc(handle, "pxm_shutdown");
        if (shutdown_func) shutdown_func();
        PxmModuleLoader::unload(handle);
    }

    std::cout << "[Host] System halted. Goodbye, Georgii." << std::endl;
    return 0;
}
