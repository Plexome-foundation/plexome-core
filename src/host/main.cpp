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
 * Designed for Windows (.dll) and POSIX (.so / .dylib) systems.
 */
class PxmModuleLoader {
public:
    static void* load(const std::string& name) {
        std::string full_path;
#ifdef PXM_PLATFORM_WINDOWS
        // Get the directory where plexome_host.exe is located
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::filesystem::path exe_dir = std::filesystem::path(buffer).parent_path();
        
        full_path = (exe_dir / (name + ".dll")).string();
        
        void* handle = LoadLibraryA(full_path.c_str());
        if (!handle) {
            DWORD err = GetLastError();
            std::cerr << "[Host] FAILED to load: " << full_path << " (Error code: " << err << ")" << std::endl;
        }
        return handle;
#else
        // Standard POSIX loading for Linux/Mac
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

    // 1. Prepare Global Configuration
    PxmConfig config;
    config.is_seed = false;
    config.network_port = 4242;
    config.tier = PerformanceTier::STANDARD;
    config.model_path = "models/phi-3-mini-4k.gguf"; // Path for AI module
    config.work_dir = "./";

    // 2. Define modules to load according to architecture
    std::vector<std::string> module_names = { 
    "pxm_ai", 
    "pxm_network", 
    "pxm_topology", 
    "pxm_sharder", 
    "pxm_updater" 
};
    std::map<std::string, void*> loaded_modules;

    // 3. Loading & Initialization Cycle
    for (const auto& name : module_names) {
        std::cout << "[Host] Loading " << name << "..." << std::endl;
        
        void* handle = PxmModuleLoader::load(name);
        if (!handle) continue;

        // Get Module Info
        auto get_info = (PXM_GET_INFO_FUNC)PxmModuleLoader::get_proc(handle, "pxm_get_info");
        if (get_info) {
            PxmModuleInfo info = get_info();
            std::cout << "  >> Module: " << info.name << " v" << info.version << std::endl;
            std::cout << "  >> Info:   " << info.description << std::endl;
        }

        // Initialize Module with Config
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
        } else {
            std::cerr << "  >> Error: Entry point 'pxm_init' not found!" << std::endl;
            PxmModuleLoader::unload(handle);
        }
    }

    std::cout << "[Host] " << loaded_modules.size() << " modules active. Swarm node ready." << std::endl;
    std::cout << "---------------------------------------" << std::endl;
    std::cout << "Press Enter to shutdown the node..." << std::endl;
    std::cin.get();

    // 4. Graceful Shutdown Cycle
    std::cout << "[Host] Shutting down modules..." << std::endl;
    for (auto const& [name, handle] : loaded_modules) {
        auto shutdown_func = (PXM_SHUTDOWN_FUNC)PxmModuleLoader::get_proc(handle, "pxm_shutdown");
        if (shutdown_func) {
            shutdown_func();
        }
        PxmModuleLoader::unload(handle);
        std::cout << "[Host] " << name << " unloaded." << std::endl;
    }

    std::cout << "[Host] System halted. Goodbye, Georgii." << std::endl; //
    return 0;
}
