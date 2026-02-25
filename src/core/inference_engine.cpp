#include "inference_engine.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace plexome {

class LlamaEngine : public InferenceEngine {
public:
    LlamaEngine() : is_loaded_(false) {}

    bool load_model(const std::string& path) override {
        if (path.empty()) return false;
        model_path_ = path;
        is_loaded_ = true;
        return true;
    }

    std::string predict(const std::string& prompt) override {
        if (!is_loaded_) return "Error: Model not loaded.";
        // Имитируем время генерации ответа (2 секунды)
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return "I processed your prompt: '" + prompt + "' using " + model_path_;
    }

    std::vector<float> process_layer_slice(const std::vector<float>& data) override {
        return data; 
    }

    bool is_loaded() const override {
        return is_loaded_;
    }

private:
    std::string model_path_;
    bool is_loaded_;
};

std::unique_ptr<InferenceEngine> InferenceEngine::create() {
    return std::make_unique<LlamaEngine>();
}

} // namespace plexome
