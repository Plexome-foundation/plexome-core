#include "plexome_types.h"
#include <iostream>
#include <memory>

/**
 * Main class representing a Plexome Network participant
 */
class PlexomeNode {
public:
    PlexomeNode(plexome::NodeRole role) {
        identity_.role = role;
        std::cout << "[Plexome] Initializing Swarm Node..." << std::endl;
    }

    /**
     * Start the node services and networking
     */
    void start() {
        // Step 1: Initialize Identity (Load or generate cryptographic keys)
        if (!setup_identity()) {
            std::cerr << "[Error] Failed to initialize PeerID." << std::endl;
            return;
        }

        // Step 2: Open Network Port 7539 (libp2p for discovery, QUIC for data)
        if (!init_networking()) {
            std::cerr << "[Error] Could not bind to port " << plexome::DEFAULT_PORT << std::endl;
            return;
        }

        // Step 3: Enter the Discovery Phase
        std::cout << "[Plexome] Node is live. Searching for peers on port " 
                  << plexome::DEFAULT_PORT << "..." << std::endl;
        
        run_loop();
    }

private:
    plexome::NodeIdentity identity_;

    /**
     * Generate or load node's PeerID based on Ed25519 keys
     */
    bool setup_identity() {
        // TODO: Implement key storage/loading logic
        identity_.id = "pxm_node_alpha_stable_01"; 
        std::cout << "[Identity] Assigned PeerID: " << identity_.id << std::endl;
        return true;
    }

    /**
     * Setup transport layer (libp2p stack + MSQUIC listener)
     */
    bool init_networking() {
        // TODO: Initialize libp2p GossipSub and MSQUIC Storage Bus
        std::cout << "[Network] Transport layer initialized on port " << plexome::DEFAULT_PORT << std::endl;
        return true;
    }

    /**
     * Main event loop for task polling and shard streaming
     */
    void run_loop() {
        std::cout << "[Core] Node state: ACTIVE. Awaiting swarm tasks..." << std::endl;
        // The node will now poll for tasks (Pull Model) and sync state
    }
};

int main() {
    // Starting a TITAN node by default
    PlexomeNode node(plexome::NodeRole::Titan);
    node.start();
    
    return 0;
}
