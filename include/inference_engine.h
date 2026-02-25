#pragma once

#include <string>
#include <vector>
#include <memory>

namespace plexome {

class InferenceEngine {
public:
    virtual ~InferenceEngine() = default;

    virtual bool load_model(const std::string& path) = 0;
    virtual std::string predict(const std::string& prompt) = 0;
    virtual std::vector<float> process_layer_slice(const std::vector<float>& data) = 0;
    virtual bool is_loaded() const = 0; // НОВЫЙ МЕТОД

    static std::unique_ptr<InferenceEngine> create();
};

} // namespace plexome
