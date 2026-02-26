/**
 * Plexome Core v2.0 - AI Engine (Inference Edition)
 * Author: Georgii
 * Handles llama.cpp lifecycle, VRAM offloading, and real-time token generation.
 */

#include "../../include/module_interface.h"
#include "llama.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

// Internal state of the AI module
struct AiState {
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    bool model_ready = false;
    std::string last_response; 
};

static AiState g_state;

extern "C" {
    /**
     * Provides metadata for the AI module.
     */
    PXM_API PxmModuleInfo pxm_get_info() {
        return {
            "Plexome AI Engine",
            "2.2.0-stable",
            "Hardware-accelerated inference with resilient model loading."
        };
    }

    /**
     * Initializes the llama.cpp backend and loads the model if present.
     */
    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        if (!config) return PxmStatus::ERROR_INIT_FAILED;

        std::cout << "[AI] Initializing llama.cpp backend..." << std::endl;
        llama_backend_init();

        // 1. Resilient check: Don't crash if model is missing
        if (!config->model_path || !fs::exists(config->model_path)) {
            std::cout << "[AI] WARNING: Model file not found at " << (config->model_path ? config->model_path : "NULL") << std::endl;
            std::cout << "[AI] Transitioning to Passive Mode. Waiting for pxm_updater." << std::endl;
            g_state.model_ready = false;
            return PxmStatus::OK; 
        }

        // 2. Model Loading Parameters
        auto mparams = llama_model_default_params();
        
        // Full VRAM offload for TITAN Tier nodes
        if (config->tier >= PerformanceTier::TITAN) {
            mparams.n_gpu_layers = 99; // Request all layers for GPU acceleration
            std::cout << "[AI] Tier 2 (Titan) detected: Enabling GPU acceleration." << std::endl;
        }

        g_state.model = llama_load_model_from_file(config->model_path, mparams);
        if (!g_state.model) {
            std::cerr << "[AI] CRITICAL: Failed to load model weights even though file exists." << std::endl;
            return PxmStatus::ERROR_INIT_FAILED;
        }

        // 3. Context Configuration
        auto cparams = llama_context_default_params();
        cparams.n_ctx = 2048;   // Context window size
        cparams.n_batch = 512;  // Batch size for processing
        cparams.n_threads = 8;  // Thread count for CPU fallback

        g_state.ctx = llama_new_context_with_model(g_state.model, cparams);
        if (!g_state.ctx) {
            std::cerr << "[AI] Failed to create llama context." << std::endl;
            return PxmStatus::ERROR_INIT_FAILED;
        }

        g_state.model_ready = true;
        std::cout << "[AI] System ready. Hardware acceleration active where available." << std::endl;

        return PxmStatus::OK;
    }

    /**
     * Performs text generation based on the provided prompt.
     */
    PXM_API const char* pxm_generate(const char* prompt) {
        if (!g_state.model_ready || !g_state.ctx) {
            return "[System] AI engine is offline: Model weights missing. Syncing via P2P...";
        }

        // Clear previous state
        g_state.last_response = "";
        
        // 1. Tokenize input
        std::vector<llama_token> tokens;
        tokens.resize(strlen(prompt) + 1);
        int n_tokens = llama_tokenize(g_state.model, prompt, strlen(prompt), tokens.data(), tokens.size(), true, true);
        tokens.resize(n_tokens);

        // 2. Prepare batch for processing
        llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size(), 0, 0);

        // 3. Generation loop (Token-by-token)
        int n_cur = 0;
        int n_max = 256; // Generation limit

        while (n_cur < n_max) {
            // Evaluate the batch
            if (llama_decode(g_state.ctx, batch)) {
                return "[Error] Decoding failed.";
            }

            // Simple Greedy Sampling: Select token with highest logit
            auto n_vocab = llama_n_vocab(g_state.model);
            auto* logits = llama_get_logits_ith(g_state.ctx, batch.n_tokens - 1);
            
            llama_token next_token = 0;
            float max_logit = -1e10;
            for (int i = 0; i < n_vocab; i++) {
                if (logits[i] > max_logit) {
                    max_logit = logits[i];
                    next_token = i;
                }
            }

            // Stop if End Of Sentence reached
            if (next_token == llama_token_eos(g_state.model)) break;

            // Convert token to piece of text
            char buf[128];
            int n = llama_token_to_piece(g_state.model, next_token, buf, sizeof(buf));
            if (n > 0) g_state.last_response.append(buf, n);

            // Prepare for next token
            batch = llama_batch_get_one(&next_token, 1, tokens.size() + n_cur, 0);
            n_cur++;
        }

        return g_state.last_response.c_str();
    }

    /**
     * Clean up resources on shutdown.
     */
    PXM_API void pxm_shutdown() {
        if (g_state.ctx) {
            llama_free(g_state.ctx);
            g_state.ctx = nullptr;
        }
        if (g_state.model) {
            llama_free_model(g_state.model);
            g_state.model = nullptr;
        }
        llama_backend_free();
        std::cout << "[AI] Resources cleared and GPU memory released." << std::endl;
    }
}
