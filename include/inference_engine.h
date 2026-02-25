#pragma once
#include "plexome_types.h"
#include <string>
#include <functional>

namespace plexome {

    /**
     * Interface for the LLM execution core.
     * Supports both CPU and GPU backends via GGUF.
     */
    class InferenceEngine {
    public:
        virtual ~InferenceEngine() = default;

        // Load model weights into memory (RAM or VRAM)
        virtual bool load_model(const std::string& model_path) = 0;

        // Execute a programming-focused prompt
        virtual std::string predict(const std::string& prompt, float temperature = 0.2f) = 0;

        // Check if GPU acceleration is active
        virtual bool is_gpu_accelerated() const = 0;
    };

    /**
     * Factory to create the engine based on hardware detection.
     */
    class EngineFactory {
    public:
        static std::unique_ptr<InferenceEngine> create_default();
    };
}
