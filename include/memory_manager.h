#pragma once
#include "plexome_types.h"
#include <map>
#include <mutex>

namespace plexome {

    struct MemoryStats {
        uint64_t total_vram;
        uint64_t used_vram;
        uint64_t total_ram;
        uint64_t used_ram;
    };

    class MemoryManager {
    public:
        MemoryManager(uint64_t vram_limit, uint64_t ram_limit);

        // Allocates space for a shard in the fastest available tier
        bool pin_shard(const ShardID& id, uint64_t size, bool prioritize_vram = true);
        
        // Releases shard from memory
        void release_shard(const ShardID& id);

        // Get current memory pressure/stats
        MemoryStats get_stats() const;

    private:
        uint64_t vram_limit_;
        uint64_t ram_limit_;
        uint64_t current_vram_usage_ = 0;
        uint64_t current_ram_usage_ = 0;

        std::map<ShardID, bool> active_shards_; // maps ShardID to "is_in_vram"
        mutable std::mutex mtx_;
    };
}
