/**
 * Plexome Core v2.0 - AI Engine with VRAM Support
 * This module manages llama.cpp lifecycle and GPU offloading.
 */

#include "../../include/module_interface.h"
#include "llama.h"
#include <iostream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

struct AiState {
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    bool gpu_active = false;
};

static AiState g_state;

extern "C" {
    PXM_API PxmModuleInfo pxm_get_info() {
        return {
            "Plexome AI Engine",
            "2.1.0-beta",
            "Neural inference with VRAM acceleration."
        };
    }

    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        if (!config || !config->model_path) return PxmStatus::ERROR_INIT_FAILED;

        std::cout << "[AI] Initializing hardware-accelerated backend..." << std::endl;
        llama_backend_init();

        // 1. Model Parameters with GPU support
        auto mparams = llama_model_default_params();
        
        // If Tier is TITAN or higher, offload all layers to VRAM
        if (config->tier >= PerformanceTier::TITAN) {
            mparams.n_gpu_layers = 99; // Request all layers for Phi-3 (usually ~33)
            std::cout << "[AI] Performance mode: TITAN. Attempting full VRAM offload." << std::endl;
        }

        // 2. Load Model
        g_state.model = llama_load_model_from_file(config->model_path, mparams);
        if (!g_state.model) {
            std::cerr << "[AI] CRITICAL: Failed to load model weights!" << std::endl;
            return PxmStatus::ERROR_INIT_FAILED;
        }

        // 3. Context Parameters
        auto cparams = llama_context_default_params();
        cparams.n_ctx = 2048; // Context window for Phi-3
        cparams.n_batch = 512;

        g_state.ctx = llama_new_context_with_model(g_state.model, cparams);
        if (!g_state.ctx) {
            std::cerr << "[AI] Failed to create execution context." << std::endl;
            return PxmStatus::ERROR_INIT_FAILED;
        }

        // Check if we requested GPU offloading
        if (mparams.n_gpu_layers > 0) {
            std::cout << "[AI] Acceleration requested: Attempted to offload layers to VRAM." << std::endl;
            g_state.gpu_active = true;
        } else {
            std::cout << "[AI] Running in CPU-only mode." << std::endl;
        }

        return PxmStatus::OK;
    }

    /**
     * Entry point for text generation
     */
    PXM_API const char* pxm_generate(const char* prompt) {
        // Simplified generation for first test
        // Real implementation will involve tokenization loop
        return "Plexome AI is ready to think..."; 
    }

    PXM_API void pxm_shutdown() {
        if (g_state.ctx) llama_free(g_state.ctx);
        if (g_state.model) llama_free_model(g_state.model);
        llama_backend_free();
        std::cout << "[AI] VRAM released. System offline." << std::endl;
    }
}
