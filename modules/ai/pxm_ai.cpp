/**
 * Plexome Core v2.0 - AI Inference Module
 * Developed by Georgii
 * * This module wraps llama.cpp to provide high-performance AI inference 
 * for the decentralized Plexome swarm network.
 */

#include "../../include/module_interface.h"
#include "llama.h"
#include <iostream>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

// Internal state to hold llama.cpp pointers
struct AiModuleState {
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    bool is_initialized = false;
};

// Global state for this DLL instance
static AiModuleState g_ai_state;

extern "C" {

    /**
     * Provides module metadata to the Plexome Orchestrator.
     */
    PXM_API PxmModuleInfo pxm_get_info() {
        PxmModuleInfo info;
        info.name = "Plexome AI Engine";
        info.version = "2.0.1-alpha";
        info.description = "Hardware-accelerated inference module based on llama.cpp core.";
        return info;
    }

    /**
     * Initializes the llama.cpp backend and verifies model existence.
     */
    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        if (!config) return PxmStatus::ERROR_INIT_FAILED;

        std::cout << "[AI] Initializing llama.cpp backend..." << std::endl;
        
        // 1. Initialize llama global backend (required once)
        llama_backend_init();

        // 2. Verify model path existence
        if (config->model_path == nullptr || strlen(config->model_path) == 0) {
            std::cerr << "[AI] Error: No model path provided in configuration." << std::endl;
            return PxmStatus::ERROR_INIT_FAILED;
        }

        fs::path model_path(config->model_path);
        if (!fs::exists(model_path)) {
            std::cerr << "[AI] CRITICAL ERROR: Model file not found at: " << fs::absolute(model_path) << std::endl;
            std::cerr << "[AI] Please ensure the model file is downloaded to the 'models' directory." << std::endl;
            // Returning error prevents the host from proceeding with an invalid state
            return PxmStatus::ERROR_INIT_FAILED; 
        }

        std::cout << "[AI] Model file verified: " << model_path.filename() << std::endl;
        
        // Note: Actual llama_load_model_from_file will be implemented in the "Parrot Meter" 
        // benchmark module to measure initial performance.

        g_ai_state.is_initialized = true;
        return PxmStatus::OK;
    }

    /**
     * Gracefully releases all allocated AI resources and frees the llama backend.
     */
    PXM_API void pxm_shutdown() {
        std::cout << "[AI] Shutting down AI engine..." << std::endl;

        if (g_ai_state.ctx) {
            llama_free(g_ai_state.ctx);
            g_ai_state.ctx = nullptr;
        }

        if (g_ai_state.model) {
            llama_free_model(g_ai_state.model);
            g_ai_state.model = nullptr;
        }

        // Global cleanup of llama.cpp resources
        llama_backend_free();
        
        g_ai_state.is_initialized = false;
        std::cout << "[AI] Llama backend successfully freed. Goodbye, Georgii." << std::endl;
    }
}
