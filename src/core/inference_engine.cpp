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
        // Раскомментируй, если будешь собирать с поддержкой GPU
        // model_params.n_gpu_layers = 99; 
        
        model_ = llama_load_model_from_file(path.c_str(), model_params);
        if (!model_) {
            std::cerr << "[AI Core] CRITICAL: Failed to load model." << std::endl;
            return false;
        }

        // В новом API llama.cpp словарь извлекается отдельно от модели
        vocab_ = llama_model_get_vocab(model_);
        if (!vocab_) {
            std::cerr << "[AI Core] CRITICAL: Failed to get vocabulary from model." << std::endl;
            return false;
        }

        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = 2048; // Размер контекстного окна (память модели)
        
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

        std::cout << "[AI Core] Tokenizing prompt..." << std::endl;

        // 1. Токенизация текста (теперь используем vocab_)
        std::vector<llama_token> tokens;
        tokens.resize(prompt.size() + 4);
        int n_tokens = llama_tokenize(vocab_, prompt.c_str(), (int32_t)prompt.length(), tokens.data(), (int32_t)tokens.size(), true, true);
        if (n_tokens < 0) {
            tokens.resize(-n_tokens);
            n_tokens = llama_tokenize(vocab_, prompt.c_str(), (int32_t)prompt.length(), tokens.data(), (int32_t)tokens.size(), true, true);
        }
        tokens.resize(n_tokens);

        // 2. Загрузка промпта в контекст модели (новый API llama_batch)
        llama_batch batch = llama_batch_get_one(tokens.data(), n_tokens);
        if (llama_decode(ctx_, batch)) {
            return "Error: Failed to decode prompt.";
        }

        std::cout << "[AI Core] Generating response..." << std::endl;

        // 3. Цикл генерации ответа (токен за токеном)
        std::string result = "";
        int n_cur = n_tokens;
        int n_predict = 256; // Максимальная длина ответа
        const int n_vocab = llama_n_vocab(vocab_);

        for (int i = 0; i < n_predict; i++) {
            // Получаем вероятности для следующего слова
            float* logits = llama_get_logits_ith(ctx_, batch.n_tokens - 1);
            
            // Выбираем самое вероятное слово (Greedy Decoding)
            llama_token new_token_id = 0;
            float max_logit = logits[0];
            for (int v = 1; v < n_vocab; v++) {
                if (logits[v] > max_logit) {
                    max_logit = logits[v];
                    new_token_id = v;
                }
            }

            // Если модель решила, что мысль закончена — прерываем цикл
            if (new_token_id == llama_token_eos(vocab_)) break;

            // Конвертируем число обратно в текст и добавляем к результату
            char buf[128];
            int n = llama_token_to_piece(vocab_, new_token_id, buf, sizeof(buf), 0, false);
            if (n > 0) result += std::string(buf, n);

            // Скармливаем сгенерированное слово обратно в модель для следующего шага
            batch = llama_batch_get_one(&new_token_id, 1);
            batch.pos[0] = n_cur; // Указываем правильную позицию в контексте
            
            if (llama_decode(ctx_, batch)) break;
            
            n_cur += 1;
        }

        return result;
    }

    std::vector<float> process_layer_slice(const std::vector<float>& data) override {
        return data; 
    }

    bool is_loaded() const override { return is_loaded_; }

private:
    llama_model* model_;
    llama_context* ctx_;
    const llama_vocab* vocab_; // Добавлен указатель на словарь
    std::string model_path_;
    bool is_loaded_;
};

std::unique_ptr<InferenceEngine> InferenceEngine::create() {
    return std::make_unique<LlamaEngine>();
}

} // namespace plexome
