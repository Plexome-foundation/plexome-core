/**
 * Plexome Core v2.0 - AI Engine (Modern Contextual Edition)
 * Author: Georgii
 * Updated for llama.cpp b2800+ API (Vocab Split & KV Cache management).
 */

#define NOMINMAX
#include "../../include/module_interface.h"
#include "llama.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>

namespace fs = std::filesystem;

/**
 * Internal state for the AI module.
 * Stores model, vocab object, context, and the persistent token counter.
 */
struct AiState {
    llama_model* model = nullptr;
    const struct llama_vocab* vocab = nullptr; 
    llama_context* ctx = nullptr;
    bool model_ready = false;
    int32_t n_past = 0; // Tracks tokens currently stored in KV cache
    std::string last_response; 
};

static AiState g_state;

extern "C" {
    PXM_API PxmModuleInfo pxm_get_info() {
        return {
            "Plexome AI Engine",
            "2.4.1-stable",
            "Context-aware engine with modern KV cache management."
        };
    }

    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        if (!config) return PxmStatus::ERROR_INIT_FAILED;

        std::cout << "[AI] Initializing persistent context backend..." << std::endl;
        llama_backend_init();

        if (!config->model_path || !fs::exists(config->model_path)) {
            std::cout << "[AI] WARNING: Weights missing. Passive mode enabled." << std::endl;
            g_state.model_ready = false;
            return PxmStatus::OK; 
        }

        auto mparams = llama_model_default_params();
        if (config->tier >= PerformanceTier::TITAN) {
            mparams.n_gpu_layers = 99; // Full VRAM offload for Titan nodes
        }

        g_state.model = llama_load_model_from_file(config->model_path, mparams);
        if (!g_state.model) return PxmStatus::ERROR_INIT_FAILED;

        g_state.vocab = llama_model_get_vocab(g_state.model);
        
        auto cparams = llama_context_default_params();
        cparams.n_ctx = 2048; 
        cparams.n_batch = 512;

        g_state.ctx = llama_new_context_with_model(g_state.model, cparams);
        if (!g_state.ctx) return PxmStatus::ERROR_INIT_FAILED;

        g_state.n_past = 0; 
        g_state.model_ready = true;
        std::cout << "[AI] Memory system ready (Context: 2048 tokens)." << std::endl;

        return PxmStatus::OK;
    }

    PXM_API const char* pxm_generate(const char* prompt) {
        if (!g_state.model_ready || !g_state.vocab || !g_state.ctx) {
            return "[System] AI engine offline.";
        }

        g_state.last_response = "";
        
        // 1. Tokenization using vocab object
        std::vector<llama_token> tokens;
        tokens.resize(strlen(prompt) + 16);
        int n_tokens = llama_tokenize(g_state.vocab, prompt, (int)strlen(prompt), tokens.data(), (int)tokens.size(), true, true);
        tokens.resize(n_tokens);

        if (tokens.empty()) return "";

        // 2. Context Management: Auto-clear if full
        if (g_state.n_past + n_tokens > 2048) {
            std::cout << "[AI] Context limit reached. Flushing KV cache." << std::endl;
            // Modern replacement for llama_kv_cache_clear
            llama_kv_cache_seq_rm(g_state.ctx, -1, -1, -1); 
            g_state.n_past = 0;
        }

        // 3. Initialize batch for current prompt
        int32_t n_alloc = (std::max)(n_tokens, 512);
        llama_batch batch = llama_batch_init(n_alloc, 0, 1);
        
        for (int i = 0; i < n_tokens; i++) {
            batch.token[i] = tokens[i];
            batch.pos[i]   = g_state.n_past + i; // Position tokens relative to history
            batch.n_seq_id[i] = 1;
            batch.seq_id[i][0] = 0;
            batch.logits[i] = false;
        }
        batch.n_tokens = n_tokens;
        batch.logits[batch.n_tokens - 1] = true;

        int n_cur = 0;
        int n_max = 256; 

        // 4. Inference loop
        while (n_cur < n_max) {
            if (llama_decode(g_state.ctx, batch)) break;

            auto* logits = llama_get_logits_ith(g_state.ctx, batch.n_tokens - 1);
            if (!logits) break; 
            
            llama_token next_token = 0;
            float max_logit = -1e10;
            for (int i = 0; i < llama_n_vocab(g_state.vocab); i++) {
                if (logits[i] > max_logit) {
                    max_logit = logits[i];
                    next_token = i;
                }
            }

            if (next_token == llama_token_eos(g_state.vocab)) break;

            char buf[256];
            int n = llama_token_to_piece(g_state.vocab, next_token, buf, sizeof(buf), 0, true);
            if (n > 0) {
                std::string piece(buf, n);
                // Filtering assistant/end tags
                if (piece.find("<|assistant|>") == std::string::npos && 
                    piece.find("<|end|>") == std::string::npos) {
                    g_state.last_response += piece;
                }
            }

            // Update batch for next token
            batch.token[0]  = next_token;
            batch.pos[0]    = g_state.n_past + n_tokens + n_cur;
            batch.n_tokens  = 1;
            batch.logits[0] = true; 
            
            n_cur++;
        }

        // 5. Update the persistent position counter
        g_state.n_past += (n_tokens + n_cur);

        llama_batch_free(batch);
        return g_state.last_response.c_str();
    }

    PXM_API void pxm_shutdown() {
        if (g_state.ctx) llama_free(g_state.ctx);
        if (g_state.model) llama_free_model(g_state.model);
        llama_backend_free();
        std::cout << "[AI] Resources released." << std::endl;
    }
}
