/**
 * Plexome Core v2.0 - Orchestrator (Host)
 * Author: Georgii
 * * This is the central brain of the node. It manages the lifecycle of all 
 * 10 architectural modules and coordinates the decentralized swarm logic.
 */

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <map>

// Plexome internal headers
#include "pxm_platform.h"
#include "module_interface.h"
#include "plexome_types.h"

/**
 * PxmModuleLoader: Handles cross-platform dynamic library operations.
 */
class PxmModuleLoader {
public:
    static void* load(const std::string& name) {
        std::string full_path;
#ifdef PXM_PLATFORM_WINDOWS
        // Resolve path relative to the executable directory
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::filesystem::path exe_dir = std::filesystem::path(buffer).parent_path();
        
        full_path = (exe_dir / (name + ".dll")).string();
        
        void* handle = LoadLibraryA(full_path.c_str());
        if (!handle) {
            DWORD err = GetLastError();
            std::cerr << "[Host] FAILED to load: " << full_path << " (Error: " << err << ")" << std::endl;
        }
        return handle;
#else
        full_path = "./" + name + ".so";
        void* handle = dlopen(full_path.c_str(), RTLD_NOW);
        if (!handle) {
            std::cerr << "[Host] FAILED to load: " << full_path << " (" << dlerror() << ")" << std::endl;
        }
        return handle;
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
    std::cout << "[Host] System startup initiated." << std::endl;

    // 1. Prepare Initial Configuration
    PxmConfig config;
    config.is_seed = false;
    config.network_port = 4242;
    config.tier = PerformanceTier::POTATO; // Default safe tier
    config.model_path = "models/phi-3-mini-4k.gguf"; 
    config.work_dir = "./";

    // 2. Define modules order
    // pxm_bench is FIRST to determine the hardware tier before other modules start
  std::vector<std::string> module_names = { 
    "pxm_bench", 
    "pxm_ai", 
    "pxm_network", 
    "pxm_topology", 
    "pxm_sharder", 
    "pxm_updater",
    "pxm_security",
    "pxm_dashboard" 
};
    
    std::map<std::string, void*> loaded_modules;

    // 3. Loading & Benchmarking Cycle
    for (const auto& name : module_names) {
        std::cout << "[Host] Loading " << name << "..." << std::endl;
        
        void* handle = PxmModuleLoader::load(name);
        if (!handle) continue;

        // Get Info
        auto get_info = (PXM_GET_INFO_FUNC)PxmModuleLoader::get_proc(handle, "pxm_get_info");
        if (get_info) {
            PxmModuleInfo info = get_info();
            std::cout << "  >> Module: " << info.name << " v" << info.version << std::endl;
        }

        // SPECIAL CASE: Point 8 - Parrot Meter (Benchmarking)
        if (name == "pxm_bench") {
            typedef PerformanceTier (*PXM_RUN_BENCH_FUNC)();
            auto run_bench = (PXM_RUN_BENCH_FUNC)PxmModuleLoader::get_proc(handle, "pxm_run_benchmark");
            if (run_bench) {
                config.tier = run_bench(); // Run test and update global config tier
                std::cout << "  >> Bench Verdict: Hardware Tier assigned as [" << (int)config.tier << "]" << std::endl;
            }
        }

        // Initialize Module with LATEST Config
        auto init_func = (PXM_INIT_FUNC)PxmModuleLoader::get_proc(handle, "pxm_init");
        if (init_func) {
            PxmStatus status = init_func(&config);
            if (status == PxmStatus::OK) {
                std::cout << "  >> Status: Successfully initialized." << std::endl;
                loaded_modules[name] = handle;
            } else {
                std::cerr << "  >> Status: Initialization FAILED." << std::endl;
                PxmModuleLoader::unload(handle);
            }
        }
    }

    std::cout << "---------------------------------------" << std::endl;
    std::cout << "[Host] Swarm node active with [" << (int)config.tier << "] status." << std::endl;
    std::cout << "[Host] " << loaded_modules.size() << " modules linked and operational." << std::endl;
    std::cout << "Press Enter to shutdown the node..." << std::endl;
    std::cin.get();

    // 4. Graceful Shutdown
    std::cout << "[Host] Initiating graceful shutdown..." << std::endl;
    for (auto const& [name, handle] : loaded_modules) {
        auto shutdown_func = (PXM_SHUTDOWN_FUNC)PxmModuleLoader::get_proc(handle, "pxm_shutdown");
        if (shutdown_func) shutdown_func();
        PxmModuleLoader::unload(handle);
    }

    std::cout << "[Host] System halted. Goodbye, Georgii." << std::endl; //
    return 0;
}
