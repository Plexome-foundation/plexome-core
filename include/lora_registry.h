#pragma once
#include "plexome_types.h"
#include <map>
#include <string>
#include <vector>

namespace plexome {

    struct LoRaMetadata {
        std::string lora_id;        // e.g., "lora_alletra_tuning_v1"
        ShardID checksum;           // SHA-256 of the delta file
        std::string base_model;     // e.g., "Llama-3-8B"
        uint64_t size_bytes;
        std::vector<PeerID> owners; // Who has this file right now
    };

    /**
     * Tracks distributed knowledge across the swarm.
     */
    class LoRaRegistry {
    public:
        // Called when our GossipService receives news about a new LoRA
        void register_discovered_lora(const LoRaMetadata& meta);

        // Get a list of LoRAs that we need to download to be fully synced
        std::vector<LoRaMetadata> get_missing_loras(const std::string& current_base_model) const;

        // Merges a downloaded LoRA into the active inference engine
        bool apply_lora_to_engine(const std::string& lora_id, class InferenceEngine* engine);

    private:
        std::map<std::string, LoRaMetadata> known_loras_;
    };
}
