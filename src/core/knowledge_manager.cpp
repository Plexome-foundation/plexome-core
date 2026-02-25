#include "knowledge_manager.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ctime>

namespace plexome {

    KnowledgeManager::KnowledgeManager(const std::string& path) : base_path_(path) {
        // Ensure knowledge directory exists for storage manuals
        if (!std::filesystem::exists(base_path_)) {
            std::filesystem::create_directories(base_path_);
        }
    }

    std::vector<KnowledgePacket> KnowledgeManager::scan_new_data() {
        std::vector<KnowledgePacket> new_packets;
        
        if (!std::filesystem::exists(base_path_)) return new_packets;

        // Iterate through knowledge folder for .txt manuals
        for (const auto& entry : std::filesystem::directory_iterator(base_path_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                std::ifstream file(entry.path());
                if (!file.is_open()) continue;

                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                // КРИТИЧНО: Закрываем файл перед переименованием, иначе Windows заблокирует доступ
                file.close(); 
                
                if (!content.empty()) {
                    KnowledgePacket packet;
                    packet.data = content;
                    packet.source_name = entry.path().filename().string();
                    packet.timestamp = static_cast<uint64_t>(std::time(nullptr));
                    
                    new_packets.push_back(packet);
                    std::cout << "[Knowledge] Ingested manual: " << packet.source_name << std::endl;
                    
                    // Помечаем файл как обработанный, меняя расширение на .done
                    try {
                        std::filesystem::path new_path = entry.path().string() + ".done";
                        std::filesystem::rename(entry.path(), new_path);
                    } catch (const std::filesystem::filesystem_error& e) {
                        std::cerr << "[Knowledge] Error renaming file: " << e.what() << std::endl;
                    }
                }
            }
        }
        return new_packets;
    }
}
