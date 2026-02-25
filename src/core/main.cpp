#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <csignal>
#include "../../include/module_interface.h"

// Structure to track loaded modules
struct ModuleInstance {
    HMODULE handle;
    std::string name;
    PXM_SHUTDOWN_FUNC shutdown;
};

std::vector<ModuleInstance> loaded_modules;
bool should_exit = false;

// Signal handler for clean exit
void signal_handler(int signal) {
    std::cout << "\n[Host] Shutdown signal received (" << signal << ")." << std::endl;
    should_exit = true;
}

// Helper to load a DLL and pass global config
bool LoadPlexomeModule(const std::string& dll_name, bool is_seed) {
    HMODULE hMod = LoadLibraryA(dll_name.c_str());
    if (!hMod) {
        std::cerr << "[Host] Failed to load " << dll_name << " | Error: " << GetLastError() << std::endl;
        return false;
    }

    auto get_info = (PXM_GET_INFO_FUNC)GetProcAddress(hMod, "pxm_get_info");
    auto init = (PXM_INIT_FUNC)GetProcAddress(hMod, "pxm_init");
    auto shutdown = (PXM_SHUTDOWN_FUNC)GetProcAddress(hMod, "pxm_shutdown");

    if (get_info && init) {
        ModuleInfo info = get_info();
        std::cout << "[Host] Loaded: " << info.name << " v" << info.version << " (Role: " << (is_seed ? "SEED" : "PEER") << ")" << std::endl;
        
        // We pass the role to the module during initialization
        if (init(is_seed)) {
            loaded_modules.push_back({hMod, info.name, shutdown});
            return true;
        }
    }

    FreeLibrary(hMod);
    return false;
}

int main(int argc, char* argv[]) {
    // 1. Windows Network Setup (Global for the process)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 1;
    }

    // 2. Parse command line (determine Node identity)
    bool is_seed = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--seed") is_seed = true;
    }

    std::cout << "=== PLEXOME MODULAR HOST (2026) ===" << std::endl;
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // 3. Dynamic Loading of blocks
    // In our new architecture, we load the AI core and Network stack as independent DLLs
    LoadPlexomeModule("pxm_ai.dll", is_seed);
    LoadPlexomeModule("pxm_network.dll", is_seed);

    // 4. Main Host Loop
    std::cout << "[Host] Running. Press Ctrl+C to terminate." << std::endl;
    while (!should_exit) {
        Sleep(200); // Low CPU usage for orchestrator
    }

    // 5. Cleanup (Unloading blocks)
    std::cout << "[Host] Cleaning up modules..." << std::endl;
    for (auto& mod : loaded_modules) {
        if (mod.shutdown) mod.shutdown();
        FreeLibrary(mod.handle);
    }

    WSACleanup();
    std::cout << "[Host] Plexome Exit Success." << std::endl;
    return 0;
}
