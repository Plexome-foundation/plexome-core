#include "knowledge_manager.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace plexome {

    KnowledgeManager::KnowledgeManager(const std::string& path) : base_path_(path) {
        if (!std::filesystem::exists(base_path_)) {
            std::filesystem::create_directories(base_path_);
        }
    }

    std::vector<std::string> KnowledgeManager::scan_new_data() {
        std::vector<std::string> new_blocks;
        
        for (const auto& entry : std::filesystem::directory_iterator(base_path_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                // Simplified: read file and treat as one block
                std::ifstream file(entry.path());
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                if (!content.empty()) {
                    new_blocks.push_back(content);
                    // Move file to 'processed' or mark it to avoid double-processing
                }
            }
        }
        return new_blocks;
    }
}
