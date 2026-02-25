#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace plexome {

    struct KnowledgePacket {
        std::string source_name;
        std::string content;
        uint64_t timestamp;
    };

    /**
     * Watches the /knowledge directory for new data on HPE 3PAR/Primera etc.
     */
    class KnowledgeManager {
    public:
        KnowledgeManager(const std::filesystem::path& path);

        // Scans for new .txt or .md files to learn from
        std::vector<KnowledgePacket> scan_new_data();

        // Marks data as "In-Training"
        void archive_processed(const std::string& source_name);

    private:
        std::filesystem::path knowledge_base_dir_;
    };
}
