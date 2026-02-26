/**
 * Plexome Core v2.0 - AI Engine (Modern API Edition)
 * Author: Georgii
 * Supports llama.cpp Vocab Split architecture and GPU offloading.
 */

#include "../../include/module_interface.h"
#include "llama.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

/**
 * Internal module state.
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
            "2.3.1-stable",
            "Inference engine with Vocab Split support and GPU acceleration."
        };
    }

    /**
     * Initializes the llama.cpp backend and loads the model.
     */
    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        if (!config) return PxmStatus::ERROR_INIT_FAILED;

        std::cout << "[AI] Initializing backend with Vocab Split support..." << std::endl;
        llama_backend_init();

        // 1. Check if model file exists
        if (!config->model_path || !fs::exists(config->model_path)) {
            std::cout << "[AI] WARNING: Model missing. Waiting for P2P sync." << std::endl;
            g_state.model_ready = false;
            return PxmStatus::OK; 
        }

        // 2. Setup model parameters
        auto mparams = llama_model_default_params();
        
        // Fully offload to VRAM if the node is Titan tier
        if (config->tier >= PerformanceTier::TITAN) {
            mparams.n_gpu_layers = 99;
            std::cout << "[AI] Performance tier: Titan. GPU offloading enabled." << std::endl;
        }

        // 3. Load model and link vocabulary
        g_state.model = llama_load_model_from_file(config->model_path, mparams);
        if (!g_state.model) return PxmStatus::ERROR_INIT_FAILED;

        g_state.vocab = llama_model_get_vocab(g_state.model);
        if (!g_state.vocab) {
            std::cerr << "[AI] CRITICAL: Vocabulary not found in model." << std::endl;
            return PxmStatus::ERROR_INIT_FAILED;
        }

        // 4. Create execution context
        auto cparams = llama_context_default_params();
        cparams.n_ctx = 2048;
        cparams.n_batch = 512;

        g_state.ctx = llama_new_context_with_model(g_state.model, cparams);
        if (!g_state.ctx) return PxmStatus::ERROR_INIT_FAILED;

        g_state.model_ready = true;
        std::cout << "[AI] Model and Context successfully linked." << std::endl;

        return PxmStatus::OK;
    }

    /**
     * Main inference entry point.
     */
    PXM_API const char* pxm_generate(const char* prompt) {
        if (!g_state.model_ready || !g_state.vocab) {
            return "[System] Engine is offline: Model not loaded.";
        }

        g_state.last_response = "";
        
        // 1. Tokenization using the new vocab object
        std::vector<llama_token> tokens;
        tokens.resize(strlen(prompt) + 16);
        int n_tokens = llama_tokenize(g_state.vocab, prompt, (int)strlen(prompt), tokens.data(), (int)tokens.size(), true, true);
        tokens.resize(n_tokens);

        // 2. Prepare batch for decoding
        llama_batch batch = llama_batch_get_one(tokens.data(), (int)tokens.size());

        int n_cur = 0;
        int n_max = 256;

        // 3. Generation loop
        while (n_cur < n_max) {
            if (llama_decode(g_state.ctx, batch)) {
                return "[Error] Evaluation failed.";
            }

            // Simple greedy sampling
            auto n_vocab = llama_n_vocab(g_state.vocab);
            auto* logits = llama_get_logits_ith(g_state.ctx, batch.n_tokens - 1);
            
            llama_token next_token = 0;
            float max_logit = -1e10;
            for (int i = 0; i < n_vocab; i++) {
                if (logits[i] > max_logit) {
                    max_logit = logits[i];
                    next_token = i;
                }
            }

            // Break if EOS token is reached
            if (next_token == llama_token_eos(g_state.vocab)) break;

            // 4. Convert token to piece using the 6-arg signature
            char buf[256];
            int n = llama_token_to_piece(g_state.vocab, next_token, buf, sizeof(buf), 0, true);
            if (n > 0) g_state.last_response.append(buf, n);

            // 5. Update batch and advance sequence position
            batch = llama_batch_get_one(&next_token, 1);
            batch.pos[0] = (llama_pos)(tokens.size() + n_cur); 
            
            n_cur++;
        }

        return g_state.last_response.c_str();
    }

    /**
     * Release GPU and CPU resources.
     */
    PXM_API void pxm_shutdown() {
        if (g_state.ctx) llama_free(g_state.ctx);
        if (g_state.model) llama_free_model(g_state.model);
        llama_backend_free();
        std::cout << "[AI] All resources released." << std::endl;
    }
}
