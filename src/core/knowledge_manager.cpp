#include "knowledge_manager.h"
#include <fstream>
#include <iostream>

namespace plexome {

    KnowledgeManager::KnowledgeManager(const std::filesystem::path& path) 
        : knowledge_base_dir_(path) {
        if (!std::filesystem::exists(knowledge_base_dir_)) {
            std::filesystem::create_directories(knowledge_base_dir_);
        }
    }

    std::vector<KnowledgePacket> KnowledgeManager::scan_new_data() {
        std::vector<KnowledgePacket> packets;
        for (const auto& entry : std::filesystem::directory_iterator(knowledge_base_dir_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                std::ifstream ifs(entry.path());
                std::string content((std::istreambuf_iterator<char>(ifs)),
                                    (std::istreambuf_iterator<char>()));
                
                packets.push_back({entry.path().filename().string(), content, 0});
                std::cout << "[Knowledge] Found new data: " << entry.path().filename() << std::endl;
            }
        }
        return packets;
    }
}
