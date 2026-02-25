#include "plexome_types.h"
#include "bootstrap_manager.h"
#include "connection_manager.h"
#include <iostream>
#include <memory>

class PlexomeNode {
public:
    PlexomeNode(plexome::NodeRole role, bool is_seed) {
        bootstrap_ = std::make_unique<plexome::BootstrapManager>(is_seed);
        conn_manager_ = std::make_unique<plexome::ConnectionManager>();
        identity_.role = role;
    }

    void start() {
        bootstrap_->bootstrap();

        // Simulation: Recieving a connection and performing a handshake
        plexome::HandshakePacket incoming_hsh;
        incoming_hsh.version = plexome::PROTOCOL_VERSION;
        incoming_hsh.sender_id = "pxm_remote_peer_99";
        incoming_hsh.sender_role = plexome::NodeRole::Archivist;

        if (conn_manager_->handle_handshake(incoming_hsh)) {
            std::cout << "[Plexome] Connection established and verified." << std::endl;
        }
    }

private:
    plexome::NodeIdentity identity_ = {"pxm_main_01", plexome::NodeRole::Titan, 0, 0, 0};
    std::unique_ptr<plexome::BootstrapManager> bootstrap_;
    std::unique_ptr<plexome::ConnectionManager> conn_manager_;
};

int main(int argc, char* argv[]) {
    bool run_as_seed = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--seed") run_as_seed = true;
    }

    PlexomeNode node(plexome::NodeRole::Titan, run_as_seed);
    node.start();
    return 0;
}
