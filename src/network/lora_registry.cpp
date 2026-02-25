#include "lora_registry.h"
#include "inference_engine.h"
#include <iostream>

namespace plexome {

    void LoRaRegistry::register_discovered_lora(const LoRaMetadata& meta) {
        // If we haven't seen this LoRA delta before, add it to our index
        if (known_loras_.find(meta.lora_id) == known_loras_.end()) {
            known_loras_[meta.lora_id] = meta;
            std::cout << "[LoRa Registry] Network gossip: Discovered new knowledge layer -> " 
                      << meta.lora_id << " (" << meta.size_bytes / 1024 / 1024 << " MB)" << std::endl;
        }
    }

    std::vector<LoRaMetadata> LoRaRegistry::get_missing_loras(const std::string& current_base_model) const {
        std::vector<LoRaMetadata> missing;
        for (const auto& [id, meta] : known_loras_) {
            // Check if the LoRA is compatible with our currently loaded base model
            // In a full implementation, we would also check local disk to see if it's already downloaded
            if (meta.base_model == current_base_model) {
                missing.push_back(meta);
            }
        }
        return missing;
    }

    bool LoRaRegistry::apply_lora_to_engine(const std::string& lora_id, class InferenceEngine* engine) {
        if (known_loras_.find(lora_id) != known_loras_.end() && engine != nullptr) {
            std::cout << "[LoRa Registry] Hot-swapping knowledge delta '" << lora_id 
                      << "' into active inference engine." << std::endl;
            
            // Future llama.cpp integration:
            // engine->apply_lora_adapter(path_to_downloaded_lora);
            
            return true;
        }
        std::cerr << "[LoRa Registry] Failed to apply knowledge layer: " << lora_id << std::endl;
        return false;
    }
}
