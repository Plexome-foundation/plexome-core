#include "inference_engine.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

// Подключаем заголовочный файл реальной библиотеки
#include "llama.h"

namespace plexome {

class LlamaEngine : public InferenceEngine {
public:
    LlamaEngine() : model_(nullptr), ctx_(nullptr), vocab_(nullptr), is_loaded_(false) {
        // Инициализация бэкенда (CPU/GPU)
        llama_backend_init(); 
    }

    ~LlamaEngine() {
        if (ctx_) llama_free(ctx_);
        if (model_) llama_free_model(model_);
        llama_backend_free();
    }

    bool load_model(const std::string& path) override {
        std::cout << "[AI Core] Loading GGUF weights into memory from: " << path << "..." << std::endl;
        
        llama_model_params model_params = llama_model_default_params();
        // Раскомментируй строку ниже для ускорения на видеокарте (когда подключим CUDA/Vulkan)
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

        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = 2048; // Размер памяти модели (контекстное окно)
        
        ctx_ = llama_new_context_with_model(model_, ctx_params);
        if (!ctx_) {
            std::cerr << "[AI Core] CRITICAL: Failed to allocate context." << std::endl;
            return false;
        }

        model_path_ = path;
        is_loaded_ = true;
        std::cout << "[AI Core] Engine online. Model ready for inference!" << std::endl;
        return true;
    }

    std::string predict(const std::string& prompt) override {
        if (!is_loaded_ || !ctx_ || !vocab_) return "Error: Model is not initialized.";

        std::cout << "[Local AI]: [AI Core] Tokenizing prompt..." << std::endl;

        // Очищаем кэш контекста, чтобы новый запрос не перемешался со старыми
        llama_kv_cache_clear(ctx_);

        // Форматируем промпт под модель Phi-3 (чтобы она отвечала как ассистент)
        std::string formatted_prompt = "<|user|>\n" + prompt + "<|end|>\n<|assistant|>\n";

        std::vector<llama_token> tokens(formatted_prompt.size() + 4);
        int n_tokens = llama_tokenize(vocab_, formatted_prompt.c_str(), (int32_t)formatted_prompt.length(), tokens.data(), (int32_t)tokens.size(), true, true);
        if (n_tokens < 0) {
            tokens.resize(-n_tokens);
            n_tokens = llama_tokenize(vocab_, formatted_prompt.c_str(), (int32_t)formatted_prompt.length(), tokens.data(), (int32_t)tokens.size(), true, true);
        }
        tokens.resize(n_tokens);

        if (n_tokens > 2000) return "Error: Prompt is too long.";

        // БЕЗОПАСНОЕ ВЫДЕЛЕНИЕ ПАМЯТИ ПОД БАТЧ
        llama_batch batch = llama_batch_init(2048, 0, 1);
        batch.n_tokens = n_tokens;

        for (int i = 0; i < n_tokens; i++) {
            batch.token[i] = tokens[i];
            batch.pos[i] = i;
            batch.n_seq_id[i] = 1;
            batch.seq_id[i][0] = 0;
            batch.logits[i] = (i == n_tokens - 1) ? 1 : 0; // Просим логиты ТОЛЬКО для последнего токена
        }

        if (llama_decode(ctx_, batch)) {
            llama_batch_free(batch);
            return "Error: Failed to decode prompt.";
        }

        std::cout << "[AI Core] Generating response:\n>> ";

        std::string result = "";
        int n_cur = n_tokens;
        int n_predict = 512; // Максимальная длина ответа
        const int n_vocab = llama_n_vocab(vocab_);

        for (int i = 0; i < n_predict; i++) {
            // Получаем вероятности
            float* logits = llama_get_logits_ith(ctx_, batch.n_tokens - 1);
            if (!logits) {
                result += "\n[Engine Error: Logits pointer is null]";
                break;
            }
            
            // Жадный сэмплинг (Greedy Decoding) - выбираем самое вероятное слово
            llama_token new_token_id = 0;
            float max_logit = -1e9f;
            for (int v = 0; v < n_vocab; v++) {
                if (logits[v] > max_logit) {
                    max_logit = logits[v];
                    new_token_id = v;
                }
            }

            if (new_token_id == llama_token_eos(vocab_)) break;

            // Конвертируем токен в текст
            char buf[128] = {0};
            int n = llama_token_to_piece(vocab_, new_token_id, buf, sizeof(buf), 0, false);
            if (n > 0) {
                std::string piece(buf, n);
                result += piece;
                // ВЫВОД В РЕАЛЬНОМ ВРЕМЕНИ
                std::cout << piece << std::flush;
            }

            // Готовим батч для следующего шага
            batch.n_tokens = 1;
            batch.token[0] = new_token_id;
            batch.pos[0] = n_cur;
            batch.n_seq_id[0] = 1;
            batch.seq_id[0][0] = 0;
            batch.logits[0] = 1; 

            if (llama_decode(ctx_, batch)) {
                result += "\n[Engine Error: Decode loop failed]";
                break;
            }
            
            n_cur += 1;
        }

        std::cout << "\n"; // Отбивка после завершения ответа
        llama_batch_free(batch);
        return result;
    }

    std::vector<float> process_layer_slice(const std::vector<float>& data) override {
        return data; 
    }

    bool is_loaded() const override { return is_loaded_; }

private:
    llama_model* model_;
    llama_context* ctx_;
    const llama_vocab* vocab_; 
    std::string model_path_;
    bool is_loaded_;
};

std::unique_ptr<InferenceEngine> InferenceEngine::create() {
    return std::make_unique<LlamaEngine>();
}

} // namespace plexome
