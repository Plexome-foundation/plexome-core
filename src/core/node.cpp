#include "plexome_types.h"
#include "bootstrap_manager.h"
#include <iostream>
#include <memory>
#include <string>

class PlexomeNode {
public:
    PlexomeNode(plexome::NodeRole role, bool is_seed) {
        is_seed_ = is_seed;
        bootstrap_ = std::make_unique<plexome::BootstrapManager>(is_seed);
        std::cout << "[Plexome] Node initialized. Role: " 
                  << (role == plexome::NodeRole::Titan ? "TITAN" : "ARCHIVIST") << std::endl;
    }

    void start() {
        // Start bootstrap process
        bootstrap_->bootstrap();

        if (is_seed_) {
            std::cout << "[Plexome] SEED NODE is active. Serving peer lists." << std::endl;
        } else {
            std::cout << "[Plexome] Node is seeking tasks from the swarm." << std::endl;
        }
    }

private:
    bool is_seed_;
    std::unique_ptr<plexome::BootstrapManager> bootstrap_;
};

int main(int argc, char* argv[]) {
    bool run_as_seed = false;
    
    // Simple argument parsing for testing
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--seed") {
            run_as_seed = true;
        }
    }

    // Creating a node (Role is Titan for this test)
    PlexomeNode node(plexome::NodeRole::Titan, run_as_seed);
    node.start();

    return 0;
}
