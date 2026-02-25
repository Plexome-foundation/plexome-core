#include "inference_engine.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

// Include the actual llama.cpp library header
#include "llama.h"

namespace plexome {

class LlamaEngine : public InferenceEngine {
public:
    LlamaEngine() : model_(nullptr), vocab_(nullptr), is_loaded_(false), active_contexts_(0) {
        // Initialize backend (CPU/GPU)
        llama_backend_init(); 
    }

    ~LlamaEngine() {
        if (model_) llama_free_model(model_);
        llama_backend_free();
    }

    bool load_model(const std::string& path) override {
        std::cout << "[AI Core] Loading GGUF weights into memory from: " << path << "..." << std::endl;
        
        llama_model_params model_params = llama_model_default_params();
        // Uncomment the line below for GPU acceleration (once CUDA/Vulkan is configured)
        // model_params.n_gpu_layers = 99; 
        
        model_ = llama_load_model_from_file(path.c_str(), model_params);
        if (!model_) {
            std::cerr << "[AI Core] CRITICAL: Failed to load model." << std::endl;
            return false;
        }

        vocab_ = llama_model_get_vocab(model_);
        if (!vocab_) {
            std::cerr << "[AI Core] CRITICAL: Failed to get vocabulary from model." << std::endl;
            return false;
        }

        // --- CONCURRENCY LIMITS ---
        // A single context of 2048 tokens takes roughly ~100-150MB of RAM.
        // We set a safe limit here. In the future, this can be calculated dynamically 
        // based on total physical RAM minus the model weight size.
        max_concurrent_contexts_ = 4; 

        model_path_ = path;
        is_loaded_ = true;
        std::cout << "[AI Core] Engine online. Thread pool capacity: " << max_concurrent_contexts_ << " parallel requests." << std::endl;
        return true;
    }

    std::string predict(const std::string& prompt) override {
        if (!is_loaded_ || !model_ || !vocab_) return "Error: Model is not initialized.";

        // ==========================================
        // 1. DYNAMIC QUEUE & SLOT RESERVATION
        // ==========================================
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (active_contexts_ >= max_concurrent_contexts_) {
            std::cout << "\n[Swarm Queue] Node is fully loaded (" << active_contexts_ << "/" 
                      << max_concurrent_contexts_ << "). Request queued..." << std::endl;
        }
        
        // Wait until a slot frees up
        queue_cv_.wait(lock, [this]() { return active_contexts_ < max_concurrent_contexts_; });
        
        // Reserve slot
        active_contexts_++;
        lock.unlock(); // Release lock immediately so other threads can join the queue!

        // ==========================================
        // 2. THREAD-LOCAL CONTEXT ALLOCATION
        // ==========================================
        // We create a fresh, isolated KV cache for this specific thread.
        // It shares the heavy read-only 'model_' safely.
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = 2048; 
        llama_context* local_ctx = llama_new_context_with_model(model_, ctx_params);

        if (!local_ctx) {
            release_slot();
            return "Error: Failed to allocate thread-local context (Out of Memory?).";
        }

        // ==========================================
        // 3. INFERENCE PROCESS
        // ==========================================
        std::string formatted_prompt = "<|user|>\n" + prompt + "<|end|>\n<|assistant|>\n";

        std::vector<llama_token> tokens(formatted_prompt.size() + 4);
        int n_tokens = llama_tokenize(vocab_, formatted_prompt.c_str(), (int32_t)formatted_prompt.length(), tokens.data(), (int32_t)tokens.size(), true, true);
        if (n_tokens < 0) {
            tokens.resize(-n_tokens);
            n_tokens = llama_tokenize(vocab_, formatted_prompt.c_str(), (int32_t)formatted_prompt.length(), tokens.data(), (int32_t)tokens.size(), true, true);
        }
        tokens.resize(n_tokens);

        if (n_tokens > 2000) {
            llama_free(local_ctx);
            release_slot();
            return "Error: Prompt is too long.";
        }

        llama_batch batch = llama_batch_init(2048, 0, 1);
        batch.n_tokens = n_tokens;

        for (int i = 0; i < n_tokens; i++) {
            batch.token[i] = tokens[i];
            batch.pos[i] = i;
            batch.n_seq_id[i] = 1;
            batch.seq_id[i][0] = 0;
            batch.logits[i] = (i == n_tokens - 1) ? 1 : 0; 
        }

        if (llama_decode(local_ctx, batch)) {
            llama_batch_free(batch);
            llama_free(local_ctx);
            release_slot();
            return "Error: Failed to decode prompt.";
        }

        std::string result = "";
        int n_cur = n_tokens;
        int n_predict = 512; 
        const int n_vocab = llama_n_vocab(vocab_);

        for (int i = 0; i < n_predict; i++) {
            float* logits = llama_get_logits_ith(local_ctx, batch.n_tokens - 1);
            if (!logits) {
                result += "\n[Engine Error: Logits pointer is null]";
                break;
            }
            
            llama_token new_token_id = 0;
            float max_logit = -1e9f;
            for (int v = 0; v < n_vocab; v++) {
                if (logits[v] > max_logit) {
                    max_logit = logits[v];
                    new_token_id = v;
                }
            }

            if (llama_token_is_eog(vocab_, new_token_id) || new_token_id == llama_token_eos(vocab_)) break;

            char buf[128] = {0};
            int n = llama_token_to_piece(vocab_, new_token_id, buf, sizeof(buf), 0, true); 
            if (n > 0) {
                std::string piece(buf, n);
                if (piece.find("<|end|>") != std::string::npos || 
                    piece.find("<|user|>") != std::string::npos ||
                    piece.find("<|assistant|>") != std::string::npos) {
                    break;
                }
                result += piece;
                
                // Print directly to console (might overlap visually if multiple threads print at once, 
                // but the returned string 'result' will be perfectly clean for the network to send back).
                std::cout << piece << std::flush;
            }

            batch.n_tokens = 1;
            batch.token[0] = new_token_id;
            batch.pos[0] = n_cur;
            batch.n_seq_id[0] = 1;
            batch.seq_id[0][0] = 0;
            batch.logits[0] = 1; 

            if (llama_decode(local_ctx, batch)) break;
            
            n_cur += 1;
        }

        // ==========================================
        // 4. CLEANUP & RELEASE SLOT
        // ==========================================
        std::cout << "\n"; 
        llama_batch_free(batch);
        llama_free(local_ctx); // Destroy the thread-local context to free up memory
        
        release_slot(); // Notify the queue that a slot is open
        return result;
    }

    std::vector<float> process_layer_slice(const std::vector<float>& data) override {
        return data; 
    }

    bool is_loaded() const override { return is_loaded_; }

private:
    // Helper function to safely decrement the active counter and wake up waiting threads
    void release_slot() {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        active_contexts_--;
        queue_cv_.notify_one(); 
    }

    // Heavy, Read-Only Model (Shared across all threads)
    llama_model* model_;
    const llama_vocab* vocab_; 
    std::string model_path_;
    bool is_loaded_;
    
    // Concurrency Controls (The Semaphore Pattern)
    int max_concurrent_contexts_;
    std::atomic<int> active_contexts_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
};

std::unique_ptr<InferenceEngine> InferenceEngine::create() {
    return std::make_unique<LlamaEngine>();
}

} // namespace plexome
