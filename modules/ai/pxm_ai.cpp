/**
 * Plexome Core v2.0 - AI Engine (Inference Edition)
 * Author: Georgii
 * * This version includes fixes for MSVC min/max macro conflicts and 
 * manual batch management for modern llama.cpp API.
 */

#define NOMINMAX // Prevent Windows.h from defining min/max macros

#include "../../include/module_interface.h"
#include "llama.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm> // Required for std::max

namespace fs = std::filesystem;

/**
 * Internal state for the AI module.
 * Manages model weights, vocabulary object, and context.
 */
struct AiState {
    llama_model* model = nullptr;
    const struct llama_vocab* vocab = nullptr; 
    llama_context* ctx = nullptr;
    bool model_ready = false;
    std::string last_response; 
};

static AiState g_state;

extern "C" {
    /**
     * Returns module metadata.
     */
    PXM_API PxmModuleInfo pxm_get_info() {
        return {
            "Plexome AI Engine",
            "2.3.5-stable",
            "Hardware-accelerated engine with MSVC compatibility fixes."
        };
    }

    /**
     * Initializes llama.cpp backend and loads the model weights.
     */
    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        if (!config) return PxmStatus::ERROR_INIT_FAILED;

        std::cout << "[AI] Initializing backend..." << std::endl;
        llama_backend_init();

        // 1. Resilience check: avoid crashing if weights are missing
        if (!config->model_path || !fs::exists(config->model_path)) {
            std::cout << "[AI] WARNING: Model file not found. System waiting for P2P sync." << std::endl;
            g_state.model_ready = false;
            return PxmStatus::OK; 
        }

        // 2. Configure model parameters
        auto mparams = llama_model_default_params();
        
        // Tier 2 (Titan) nodes attempt to offload all layers to VRAM
        if (config->tier >= PerformanceTier::TITAN) {
            mparams.n_gpu_layers = 99;
            std::cout << "[AI] Performance tier: Titan. GPU acceleration requested." << std::endl;
        }

        // 3. Load model and extract vocabulary
        g_state.model = llama_load_model_from_file(config->model_path, mparams);
        if (!g_state.model) return PxmStatus::ERROR_INIT_FAILED;

        g_state.vocab = llama_model_get_vocab(g_state.model);
        
        auto cparams = llama_context_default_params();
        cparams.n_ctx = 2048; 
        cparams.n_batch = 512;

        g_state.ctx = llama_new_context_with_model(g_state.model, cparams);
        if (!g_state.ctx) return PxmStatus::ERROR_INIT_FAILED;

        g_state.model_ready = true;
        std::cout << "[AI] Inference engine active and hardware-linked." << std::endl;

        return PxmStatus::OK;
    }

    /**
     * Main inference entry point with explicit memory safety for batching.
     */
    PXM_API const char* pxm_generate(const char* prompt) {
        if (!g_state.model_ready || !g_state.vocab || !g_state.ctx) {
            return "[System] AI engine offline: Model not loaded.";
        }

        g_state.last_response = "";
        
        // 1. Tokenize input prompt
        std::vector<llama_token> tokens;
        tokens.resize(strlen(prompt) + 16);
        int n_tokens = llama_tokenize(g_state.vocab, prompt, (int)strlen(prompt), tokens.data(), (int)tokens.size(), true, true);
        tokens.resize(n_tokens);

        if (tokens.empty()) return "";

        // 2. Initialize batch with explicit memory allocation to prevent crashes
        // Wrap std::max in parentheses to bypass potential MSVC macro conflicts.
        int32_t n_alloc = (std::max)(n_tokens, 512);
        llama_batch batch = llama_batch_init(n_alloc, 0, 1);
        
        // Manually fill batch for the initial prompt
        for (int i = 0; i < n_tokens; i++) {
            batch.token[i] = tokens[i];
            batch.pos[i]   = i;
            batch.n_seq_id[i] = 1;
            batch.seq_id[i][0] = 0;
            batch.logits[i] = false;
        }
        batch.n_tokens = n_tokens;
        batch.logits[batch.n_tokens - 1] = true; // Request logits for the final prompt token

        int n_cur = 0;
        int n_max = 256; // Max tokens to generate per request

        // 3. Generation loop
        while (n_cur < n_max) {
            if (llama_decode(g_state.ctx, batch)) {
                break;
            }

            // Retrieve logits for the last processed token
            auto* logits = llama_get_logits_ith(g_state.ctx, batch.n_tokens - 1);
            if (!logits) break; 
            
            // Greedy sampling: pick the most likely next token
            llama_token next_token = 0;
            float max_logit = -1e10;
            for (int i = 0; i < llama_n_vocab(g_state.vocab); i++) {
                if (logits[i] > max_logit) {
                    max_logit = logits[i];
                    next_token = i;
                }
            }

            // End generation if EOS token is encountered
            if (next_token == llama_token_eos(g_state.vocab)) break;

            // Convert token to string
            char buf[256];
            int n = llama_token_to_piece(g_state.vocab, next_token, buf, sizeof(buf), 0, true);
            if (n > 0) g_state.last_response.append(buf, n);

            // Update batch for the next single-token step
            batch.token[0]  = next_token;
            batch.pos[0]    = n_tokens + n_cur;
            batch.n_tokens  = 1;
            batch.logits[0] = true; 
            
            n_cur++;
        }

        llama_batch_free(batch); // Free batch memory after generation
        return g_state.last_response.c_str();
    }

    /**
     * Shutdown and release resources.
     */
    PXM_API void pxm_shutdown() {
        if (g_state.ctx) llama_free(g_state.ctx);
        if (g_state.model) llama_free_model(g_state.model);
        llama_backend_free();
        std::cout << "[AI] Resources released." << std::endl;
    }
}
