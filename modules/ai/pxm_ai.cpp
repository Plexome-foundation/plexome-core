/**
 * Plexome Core v2.0 - AI Engine (Inference Edition)
 * Author: Georgii
 * Fixed for modern Vocab Split API with explicit batch memory allocation.
 */

#include "../../include/module_interface.h"
#include "llama.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

/**
 * Internal state for the AI module.
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
            "2.3.4-stable",
            "Hardware-accelerated engine with explicit batch management."
        };
    }

    /**
     * Initializes llama.cpp backend and loads the model.
     */
    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        if (!config) return PxmStatus::ERROR_INIT_FAILED;

        std::cout << "[AI] Initializing backend..." << std::endl;
        llama_backend_init();

        if (!config->model_path || !fs::exists(config->model_path)) {
            std::cout << "[AI] WARNING: Model file not found at " << (config->model_path ? config->model_path : "NULL") << std::endl;
            g_state.model_ready = false;
            return PxmStatus::OK; 
        }

        auto mparams = llama_model_default_params();
        if (config->tier >= PerformanceTier::TITAN) {
            mparams.n_gpu_layers = 99; // Attempt full VRAM offload
        }

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
     * Main inference loop with explicit batch allocation to prevent crashes.
     */
    PXM_API const char* pxm_generate(const char* prompt) {
        if (!g_state.model_ready || !g_state.vocab || !g_state.ctx) {
            return "[System] AI engine offline.";
        }

        g_state.last_response = "";
        std::cout << "[AI] Tokenizing input..." << std::endl;
        
        // 1. Tokenization
        std::vector<llama_token> tokens;
        tokens.resize(strlen(prompt) + 16);
        int n_tokens = llama_tokenize(g_state.vocab, prompt, (int)strlen(prompt), tokens.data(), (int)tokens.size(), true, true);
        tokens.resize(n_tokens);

        if (tokens.empty()) return "";

        // 2. Explicit batch initialization (prevents NULL pointer crashes)
        // We allocate enough space for the prompt and subsequent generation steps.
        llama_batch batch = llama_batch_init(std::max(n_tokens, 512), 0, 1);
        
        // Fill batch with prompt tokens
        for (int i = 0; i < n_tokens; i++) {
            batch.token[i] = tokens[i];
            batch.pos[i]   = i;
            batch.n_seq_id[i] = 1;
            batch.seq_id[i][0] = 0;
            batch.logits[i] = false;
        }
        batch.n_tokens = n_tokens;
        batch.logits[batch.n_tokens - 1] = true; // We need logits for the last token to start generation

        std::cout << "[AI] Decoding prompt (" << n_tokens << " tokens)..." << std::endl;

        int n_cur = 0;
        int n_max = 128; // Safety limit for testing

        // 3. Generation loop
        while (n_cur < n_max) {
            if (llama_decode(g_state.ctx, batch)) {
                std::cerr << "[AI] Decode failed!" << std::endl;
                break;
            }

            auto* logits = llama_get_logits_ith(g_state.ctx, batch.n_tokens - 1);
            if (!logits) {
                std::cerr << "[AI] CRITICAL: Logits are NULL!" << std::endl;
                break;
            }
            
            // Greedy sampling
            llama_token next_token = 0;
            float max_logit = -1e10;
            for (int i = 0; i < llama_n_vocab(g_state.vocab); i++) {
                if (logits[i] > max_logit) {
                    max_logit = logits[i];
                    next_token = i;
                }
            }

            if (next_token == llama_token_eos(g_state.vocab)) break;

            // Convert token to piece
            char buf[256];
            int n = llama_token_to_piece(g_state.vocab, next_token, buf, sizeof(buf), 0, true);
            if (n > 0) g_state.last_response.append(buf, n);

            // Prepare batch for ONLY the next single token
            batch.token[0]  = next_token;
            batch.pos[0]    = n_tokens + n_cur;
            batch.n_tokens  = 1;
            batch.logits[0] = true; 
            
            n_cur++;
        }

        llama_batch_free(batch); // Don't forget to free allocated memory!
        return g_state.last_response.c_str();
    }

    PXM_API void pxm_shutdown() {
        if (g_state.ctx) llama_free(g_state.ctx);
        if (g_state.model) llama_free_model(g_state.model);
        llama_backend_free();
        std::cout << "[AI] Resources released." << std::endl;
    }
}
