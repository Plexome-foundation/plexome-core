#pragma once
#include "plexome_types.h"
#include <string>
#include <vector>
#include <memory>

namespace plexome {

    class InferenceEngine {
    public:
        virtual ~InferenceEngine() = default;

        // Load GGUF models (Llama-3, Phi-3, etc.)
        virtual bool load_model(const std::string& path) = 0;

        // Generate technical advice or process tensors
        virtual std::string predict(const std::string& prompt) = 0;

        virtual std::vector<float> process_layer_slice(const std::vector<float>& input_tensor) = 0;
    };

    class EngineFactory {
    public:
        // Returns the best available engine (CPU or GPU based)
        static std::unique_ptr<InferenceEngine> create_default();
    };
}
