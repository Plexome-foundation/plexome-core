#pragma once
#include "plexome_types.h"
#include <string>

namespace plexome {

    // AppConfig is now defined in plexome_types.h

    class ConfigLoader {
    public:
        // Loads configuration from the specified INI or JSON file
        static AppConfig load_file(const std::string& filepath);
        
        // Generates a default configuration if the file is missing
        static void generate_default(const std::string& filepath);
    };
}
