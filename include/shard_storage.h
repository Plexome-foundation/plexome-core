#pragma once
#include "plexome_types.h"
#include <filesystem>
#include <fstream>
#include <optional>

namespace plexome {

    /**
     * Handles local disk persistence for AI model shards.
     * Maps ShardID to physical files on the NVMe/HDD.
     */
    class ShardStorage {
    public:
        explicit ShardStorage(const std::filesystem::path& base_path);

        // Writes a shard buffer to disk
        bool save_shard(const ShardID& id, const std::vector<uint8_t>& data);

        // Reads a shard from disk into a buffer
        std::optional<std::vector<uint8_t>> load_shard(const ShardID& id);

        // Checks if shard exists locally
        bool exists(const ShardID& id) const;

        // Returns total storage usage in bytes
        uint64_t get_total_usage() const;

    private:
        std::filesystem::path root_dir_;
        std::filesystem::path get_path(const ShardID& id) const;
        
        // Helper to convert ShardID (binary) to Hex string for filenames
        std::string shard_to_hex(const ShardID& id) const;
    };
}
