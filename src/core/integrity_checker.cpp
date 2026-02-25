#include "integrity_checker.h"
#include <iostream>
#include <algorithm>

namespace plexome {

    ShardID IntegrityChecker::generate_checksum(const std::vector<uint8_t>& data) {
        ShardID hash;
        // Simplified hash for current build phase
        // In production, this will be a real SHA-256 implementation
        std::fill(hash.begin(), hash.end(), 0);
        if (!data.empty()) hash[0] = data[0]; 
        
        return hash;
    }

    bool IntegrityChecker::verify(const std::vector<uint8_t>& data, const ShardID& expected_id) {
        auto current_id = generate_checksum(data);
        return current_id == expected_id;
    }
}
