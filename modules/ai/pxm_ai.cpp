/**
 * Plexome Core v2.0 - AI Engine (Dynamic CPU Edition)
 * Author: Georgii
 * Features dynamic thread allocation based on hardware concurrency
 * and NUMA awareness for optimal performance on any hardware.
 */

#define NOMINMAX
#include "../../include/module_interface.h"
#include "llama.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <thread> 
namespace fs = std::filesystem;

struct AiState {
    llama_model* model = nullptr;
    const struct llama_vocab* vocab = nullptr; 
    llama_context* ctx = nullptr;
    bool model_ready = false;
    int32_t n_past = 0; 
    std::string last_response; 
};

static AiState g_state;

void clean_output(std::string& text) {
    const std::vector<std::string> junk = {
        "<|assistant|>", "<|end|>", "<|user|>", "<|system|>",
        "## Instruction", "*Note:", "Greetings!"
    };
    for (const auto& tag : junk) {
        size_t pos = 0;
        while ((pos = text.find(tag, pos)) != std::string::npos) {
            if (tag[0] == '#' || tag[0] == '*') {
                size_t line_end = text.find('\n', pos);
                if (line_end != std::string::npos) text.erase(pos, line_end - pos + 1);
                else text.erase(pos);
            } else {
                text.erase(pos, tag.length());
            }
        }
    }
}

extern "C" {
    PXM_API PxmModuleInfo pxm_get_info() {
        return { "Plexome AI Engine", "3.0.0-dynamic", "AI with auto-scaling thread allocation." };
    }

    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        if (!config) return PxmStatus::ERROR_INIT_FAILED;
        

        llama_numa_init(GGML_NUMA_STRATEGY_DISTRIBUTE);
        llama_backend_init();

        if (!config->model_path || !fs::exists(config->model_path)) {
            g_state.model_ready = false;
            return PxmStatus::OK; 
        }

        auto mparams = llama_model_default_params();
        mparams.use_mmap = true; 
        
        if (config->tier >= PerformanceTier::TITAN) {
            mparams.n_gpu_layers = 99;
        } else {
            mparams.n_gpu_layers = 0;
        }

        g_state.model = llama_load_model_from_file(config->model_path, mparams);
        if (!g_state.model) return PxmStatus::ERROR_INIT_FAILED;

        g_state.vocab = llama_model_get_vocab(g_state.model);
        auto cparams = llama_context_default_params();
        cparams.n_ctx = 2048; 
        cparams.n_batch = 512;
      

     
        unsigned int logical_cores = std::thread::hardware_concurrency();
        if (logical_cores == 0) logical_cores = 4; 

        unsigned int physical_cores = (std::max)(1u, logical_cores / 2);

        cparams.n_threads = physical_cores;
        cparams.n_threads_batch = logical_cores;

        std::cout << "[AI] Hardware detected: " << logical_cores << " logical cores." << std::endl;
        std::cout << "[AI] Auto-scaling: " << physical_cores << " threads for generation, " 
                  << logical_cores << " threads for prompt evaluation." << std::endl;
      
        g_state.ctx = llama_new_context_with_model(g_state.model, cparams);
        if (!g_state.ctx) return PxmStatus::ERROR_INIT_FAILED;

        g_state.n_past = 0; 
        g_state.model_ready = true;
        std::cout << "[AI] Pro-level engine initialized successfully." << std::endl;
        return PxmStatus::OK;
    }

    PXM_API const char* pxm_generate(const char* prompt) {
        if (!g_state.model_ready || !g_state.ctx) return "[System] Engine offline.";

        g_state.last_response = "";
        
        std::vector<llama_token> tokens;
        tokens.resize(strlen(prompt) + 16);
        bool add_bos = (g_state.n_past == 0);
        int n_tokens = llama_tokenize(g_state.vocab, prompt, (int)strlen(prompt), tokens.data(), (int)tokens.size(), add_bos, true);
        tokens.resize(n_tokens);

        if (tokens.empty()) return "";

        int32_t n_alloc = (std::max)(n_tokens, 512);
        llama_batch batch = llama_batch_init(n_alloc, 0, 1);
        
        for (int i = 0; i < n_tokens; i++) {
            batch.token[i] = tokens[i];
            batch.pos[i]   = g_state.n_past + i; 
            batch.n_seq_id[i] = 1;
            batch.seq_id[i][0] = 0;
            batch.logits[i] = false;
        }
        batch.n_tokens = n_tokens;
        batch.logits[batch.n_tokens - 1] = true;

        int n_cur = 0;
        int n_max = 256; 

        while (n_cur < n_max) {
            if (llama_decode(g_state.ctx, batch)) break;

            g_state.n_past += batch.n_tokens; 

            auto* logits = llama_get_logits_ith(g_state.ctx, batch.n_tokens - 1);
            if (!logits) break; 
            
            llama_token next_token = 0;
            float max_logit = -1e10;
            auto n_vocab = llama_n_vocab(g_state.vocab);
            for (int i = 0; i < n_vocab; i++) {
                if (logits[i] > max_logit) {
                    max_logit = logits[i];
                    next_token = i;
                }
            }

            if (next_token == llama_token_eos(g_state.vocab)) break;

            char buf[256];
            int n = llama_token_to_piece(g_state.vocab, next_token, buf, sizeof(buf), 0, true);
            if (n > 0) g_state.last_response.append(buf, n);

            batch.token[0]  = next_token;
            batch.pos[0]    = g_state.n_past; 
            batch.n_tokens  = 1;
            batch.logits[0] = true; 
            n_cur++;
        }

        clean_output(g_state.last_response); 
        llama_batch_free(batch);
        return g_state.last_response.c_str();
    }

    PXM_API void pxm_shutdown() {
        if (g_state.ctx) llama_free(g_state.ctx);
        if (g_state.model) llama_free_model(g_state.model);
        llama_backend_free();
    }
}
