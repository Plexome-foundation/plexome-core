#include "memory_manager.h"
#include <iostream>

namespace plexome {

    MemoryManager::MemoryManager(uint64_t vram_limit, uint64_t ram_limit)
        : vram_limit_(vram_limit), ram_limit_(ram_limit) {
        std::cout << "[Memory] Manager initialized. L1 (VRAM): " << vram_limit / 1024 / 1024 << "MB, "
                  << "L2 (RAM): " << ram_limit / 1024 / 1024 << "MB" << std::endl;
    }

    bool MemoryManager::pin_shard(const ShardID& id, uint64_t size, bool prioritize_vram) {
        std::lock_guard<std::mutex> lock(mtx_);

        // Tier 1: Try VRAM
        if (prioritize_vram && (current_vram_usage_ + size <= vram_limit_)) {
            current_vram_usage_ += size;
            active_shards_[id] = true;
            std::cout << "[Memory] Shard pinned to L1 (VRAM)" << std::endl;
            return true;
        }

        // Tier 2: Fallback to RAM
        if (current_ram_usage_ + size <= ram_limit_) {
            current_ram_usage_ += size;
            active_shards_[id] = false;
            std::cout << "[Memory] Shard pinned to L2 (RAM)" << std::endl;
            return true;
        }

        std::cerr << "[Memory] Out of memory for pinning shard!" << std::endl;
        return false;
    }

    void MemoryManager::release_shard(const ShardID& id) {
        std::lock_guard<std::mutex> lock(mtx_);
        // Logic to decrease usage based on whether it was in VRAM or RAM
        active_shards_.erase(id);
    }

    MemoryStats MemoryManager::get_stats() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return {vram_limit_, current_vram_usage_, ram_limit_, current_ram_usage_};
    }
}
