#pragma once
#include "plexome_types.h"
#include <string>
#include <vector>

namespace plexome {

    // KnowledgePacket is now defined in plexome_types.h

    class KnowledgeManager {
    public:
        explicit KnowledgeManager(const std::string& path);

        // Scans the ./knowledge directory for new technical manuals
        std::vector<KnowledgePacket> scan_new_data();

    private:
        std::string base_path_;
    };
}
