#pragma once
#include <string>
#include <map>
#include <vector>
#include <cstdint>

namespace plexome {

    /**
     * Stores all node settings.
     */
    struct AppConfig {
        uint16_t port = 7539;
        uint64_t vram_limit_bytes = 4294967296; // 4GB default
        uint64_t ram_limit_bytes = 8589934592;  // 8GB default
        std::string storage_path = "./plexome_data";
        bool is_seed = false;
        std::vector<std::string> dns_seeds;
    };

    /**
     * Loads settings from a .conf file.
     */
    class ConfigLoader {
    public:
        static AppConfig load_file(const std::string& filename);
    };
}
