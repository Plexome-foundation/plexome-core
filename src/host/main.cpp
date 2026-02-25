#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include "pxm_platform.h"
#include "module_interface.h"

/**
 * Helper class to manage dynamic library handles cross-platform
 */
class PxmModuleLoader {
public:
    static void* load(const std::string& name) {
        std::string full_path;
#ifdef PXM_PLATFORM_WINDOWS
        full_path = name + ".dll";
        return LoadLibraryA(full_path.c_str());
#else
        full_path = "./lib" + name + ".so";
        return dlopen(full_path.c_str(), RTLD_NOW);
#endif
    }

    static void* get_proc(void* handle, const std::string& func_name) {
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

    // List of modules to boot up
    std::vector<std::string> modules_to_load = {"pxm_ai", "pxm_network"};
    
    for (const auto& mod_name : modules_to_load) {
        std::cout << "[Host] Loading module: " << mod_name << "..." << std::endl;
        
        void* handle = PxmModuleLoader::load(mod_name);
        if (!handle) {
            std::cerr << "[Host] FAILED to load " << mod_name << std::endl;
            continue;
        }

        // Try to get info function
        auto get_info = (PXM_GET_INFO_FUNC)PxmModuleLoader::get_proc(handle, "pxm_get_info");
        
        if (get_info) {
            PxmModuleInfo info = get_info();
            std::cout << "  >> Name: " << info.name << std::endl;
            std::cout << "  >> Ver:  " << info.version << std::endl;
            std::cout << "  >> Desc: " << info.description << std::endl;
        } else {
            std::cerr << "  >> Error: Could not find pxm_get_info in " << mod_name << std::endl;
        }

        // For now, we just unload it to test the cycle
        // In the real app, we would keep handles in a map
        PxmModuleLoader::unload(handle);
        std::cout << "[Host] Module " << mod_name << " test finished." << std::endl;
    }

    std::cout << "[Host] All tests completed. System ready." << std::endl;
    return 0;
}
