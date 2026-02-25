#pragma once
#include "plexome_types.h"
#include <vector>
#include <string>

namespace plexome {

    /**
     * Handles data integrity and shard verification.
     * Prevents corrupted or malicious weights from entering the L1/L2 cache.
     */
    class IntegrityChecker {
    public:
        // Generates a checksum for a data block (SHA-256 placeholder)
        static ShardID generate_checksum(const std::vector<uint8_t>& data);

        // Verifies if the data matches the expected ShardID
        static bool verify(const std::vector<uint8_t>& data, const ShardID& expected_id);
    };
}
