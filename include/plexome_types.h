#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace plexome {

    /**
     * Standard networking configuration
     */
    const uint16_t DEFAULT_PORT = 7539;
    const uint32_t DEFAULT_SHARD_SIZE_MB = 4;

    /**
     * Identity and Addressing
     */
    using PeerID = std::string; // Unique node identifier (Base58 encoded public key)
    using ShardID = std::array<uint8_t, 32>; // SHA-256 hash of the weight chunk

    /**
     * Node Classification in the Swarm
     */
    enum class NodeRole {
        Archivist, // High-capacity storage node (L3/Cold Storage)
        Titan,     // GPU-enabled compute node (L1/Hot Processing)
        Validator  // Consensus node for Majority Vote verification
    };

    /**
     * Node Capability Manifest
     */
    struct NodeIdentity {
        PeerID id;
        NodeRole role;
        uint64_t vram_capacity_bytes;
        uint64_t ram_capacity_bytes;
        uint32_t network_bandwidth_mbps;
    };

    /**
     * Model Shard Metadata
     */
    struct ShardMetadata {
        ShardID shard_id;
        std::string model_name;
        uint32_t layer_index;
        uint64_t version_epoch;
    };

    // ====================================================================
    // V2.0 ADDITIONS: Knowledge Injection, Benchmarking & Core Config
    // ====================================================================

    enum class PerformanceTier {
        Potato,   // Low-end/Legacy
        Standard, // Modern PC
        Titan     // Enterprise/High-end GPU
    };

    struct KnowledgePacket {
        std::string source_name;
        std::string data;
        uint64_t timestamp;
    };

    struct AppConfig {
        uint16_t port = DEFAULT_PORT;
        bool is_seed = false;
        std::string storage_path = "./data";
        uint64_t vram_limit_bytes = 0;
        uint64_t ram_limit_bytes = 0;
    };
}
