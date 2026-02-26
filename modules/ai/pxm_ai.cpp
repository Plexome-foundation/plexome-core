/**
 * Plexome Core v2.1 - AI Engine (Optimized Edition)
 * Author: Georgii (optimized by Perplexity)
 * Improvements: Configurable params, safer logits, dynamic buffers, reuse batch alloc, better EOS.
 */

#define NOMINMAX
#include "../../include/module_interface.h"
#include "llama.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;

struct AiState {
    llama_model* model = nullptr;
    const struct llama_vocab* vocab = nullptr; 
    llama_context* ctx = nullptr;
    llama_batch batch = {}; // Reuse alloc
    int32_t n_past = 0;
    std::string last_response; 
    int n_ctx = 2048;
    int n_batch_size = 512;
    int n_threads = 4;
    int n_max_tokens = 256;
    bool model_ready = false;
};

static AiState g_state;

/**
 * Strips chat template junk efficiently.
 */
void clean_output(std::string& text) {
    const std::string junk[] = {"<|assistant|>", "<|end|>", "<|user|>", "<|system|>", "## Instruction", "*Note:", "Greetings!"};
    for (const auto& tag : junk) {
        size_t pos = 0;
        while ((pos = text.find(tag, pos)) != std::string::npos) {
            text.erase(pos, tag.length());
        }
    }
    // Trim lines starting with junk
    size_t start = 0;
    while ((start = text.find('\n', start)) != std::string::npos) {
        if (start + 1 < text.size() && (text[start + 1] == '#' || text[start + 1] == '*')) {
            size_t end = text.find('\n', start + 1);
            text.erase(start + 1, (end == std::string::npos ? text.size() : end) - start - 1);
        } else {
            ++start;
        }
    }
}

extern "C" {
    PXM_API PxmModuleInfo pxm_get_info() {
        return { "Plexome AI Engine", "2.1.0-opt", "Optimized AI with config & safety." };
    }

    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        if (!config) return PxmStatus::ERROR_INIT_FAILED;
        llama_backend_init();

        if (!config->model_path || !fs::exists(config->model_path)) {
            g_state.model_ready = false;
            return PxmStatus::OK; 
        }

        auto mparams = llama_model_default_params();
        if (config->tier >= PerformanceTier::TITAN) {
            mparams.n_gpu_layers = 99;
        }

        g_state.model = llama_load_model_from_file(config->model_path, mparams);
        if (!g_state.model) return PxmStatus::ERROR_INIT_FAILED;

        g_state.vocab = llama_model_get_vocab(g_state.model);
        auto cparams = llama_context_default_params();
        g_state.n_ctx = config->max_ctx ? config->max_ctx : 2048;
        g_state.n_batch_size = config->batch_size ? config->batch_size : 512;
        g_state.n_threads = config->threads ? config->threads : 4;
        g_state.n_max_tokens = config->max_tokens ? config->max_tokens : 256;
        cparams.n_ctx = g_state.n_ctx;
        cparams.n_batch = g_state.n_batch_size;
        cparams.n_threads = g_state.n_threads;
        cparams.n_threads_batch = g_state.n_threads;
        g_state.ctx = llama_new_context_with_model(g_state.model, cparams);
        if (!g_state.ctx) return PxmStatus::ERROR_INIT_FAILED;

        // Pre-alloc batch for reuse
        g_state.batch = llama_batch_init(g_state.n_ctx, 0, 1);

        g_state.n_past = 0;
        g_state.model_ready = true;
        std::cout << "[AI] Optimized engine ready. ctx=" << g_state.n_ctx 
                  << " batch=" << g_state.n_batch_size << std::endl;
        return PxmStatus::OK;
    }

    PXM_API const char* pxm_generate(const char* prompt) {
        if (!g_state.model_ready || !g_state.ctx) return "[System] Engine offline.";

        g_state.last_response.clear();
        
        // Tokenize with dynamic sizing
        std::vector<llama_token> tokens;
        int prompt_len = strlen(prompt);
        tokens.resize(prompt_len * 4 / 3 + 16); // Heuristic + margin
        bool add_bos = (g_state.n_past == 0);
        int n_tokens = llama_tokenize(g_state.vocab, prompt, prompt_len, 
                                      tokens.data(), tokens.size(), add_bos, true);
        tokens.resize(n_tokens);
        if (tokens.empty()) return "";

        // Eval prompt in one batch
        llama_batch_clear(&g_state.batch);
        for (int i = 0; i < n_tokens; ++i) {
            g_state.batch.token[i] = tokens[i];
            g_state.batch.pos[i] = g_state.n_past + i;
            g_state.batch.n_seq_id[i] = 1;
            g_state.batch.seq_id[i][0] = 0;
            g_state.batch.logits[i] = false;
        }
        g_state.batch.n_tokens = n_tokens;
        g_state.batch.logits[n_tokens - 1] = true;

        if (llama_decode(g_state.ctx, g_state.batch)) {
            return "[System] Decode failed.";
        }
        g_state.n_past += n_tokens;

        // Greedy gen loop
        int n_cur = 0;
        llama_token eos_token = llama_token_eos(g_state.vocab);
        int n_vocab = llama_n_vocab(g_state.vocab);
        while (n_cur < g_state.n_max_tokens) {
            // Safer logits: use full array or check
            float* logits = llama_get_logits(g_state.ctx);
            if (!logits) break;

            llama_token next_token = 0;
            float max_logit = -INFINITY;
            for (int i = 0; i < n_vocab; ++i) {
                if (logits[i] > max_logit) {
                    max_logit = logits[i];
                    next_token = i;
                }
            }

            if (next_token == eos_token) break;

            // Dynamic decode with loop for multi-byte pieces
            char buf[8];
            int pos = 0;
            while (true) {
                int n = llama_token_to_piece(g_state.vocab, next_token, buf + pos, sizeof(buf) - pos, pos, true);
                if (n <= 0) break;
                pos += n;
            }
            if (pos > 0) g_state.last_response.append(buf, pos);

            // Next single-token batch
            llama_batch_clear(&g_state.batch);
            g_state.batch.token[0] = next_token;
            g_state.batch.pos[0] = g_state.n_past;
            g_state.batch.n_seq_id[0] = 1;
            g_state.batch.seq_id[0][0] = 0;
            g_state.batch.n_tokens = 1;
            g_state.batch.logits[0] = true;

            if (llama_decode(g_state.ctx, g_state.batch)) break;
            g_state.n_past += 1;
            ++n_cur;
        }

        clean_output(g_state.last_response);
        return g_state.last_response.c_str();
    }

    PXM_API void pxm_shutdown() {
        llama_batch_free(g_state.batch);
        if (g_state.ctx) llama_free(g_state.ctx);
        if (g_state.model) llama_free_model(g_state.model);
        llama_backend_free();
    }
}
