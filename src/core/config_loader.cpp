#include "config_loader.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace plexome {

    AppConfig ConfigLoader::load_file(const std::string& filename) {
        AppConfig config;
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "[Config] No config file found. Using defaults." << std::endl;
            return config;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream is_line(line);
            std::string key;
            if (std::getline(is_line, key, '=')) {
                std::string value;
                if (std::getline(is_line, value)) {
                    if (key == "port") config.port = std::stoi(value);
                    else if (key == "is_seed") config.is_seed = (value == "true");
                    else if (key == "storage_path") config.storage_path = value;
                    else if (key == "vram_limit_gb") config.vram_limit_bytes = std::stoull(value) * 1024 * 1024 * 1024;
                }
            }
        }
        std::cout << "[Config] Loaded configuration from " << filename << std::endl;
        return config;
    }
}
