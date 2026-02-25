#pragma once
#include "plexome_types.h"
#include <functional>

namespace plexome {

    /**
     * Interface for P2P communication layer.
     * Manages peer discovery and gossip-based state sync.
     */
    class NetworkBus {
    public:
        virtual ~NetworkBus() = default;

        virtual void start(uint16_t port) = 0;
        virtual void stop() = 0;

        // Broadcast metadata to the swarm (Gossip)
        virtual void broadcast_state(const std::string& message) = 0;

        // Request a specific shard from the network
        virtual void request_shard(const ShardID& id, std::function<void(const std::vector<uint8_t>&)> callback) = 0;

        // Event handler for peer discovery
        std::function<void(const PeerID&)> on_peer_connected;
    };

    /**
     * Mock implementation for today's build and testing.
     */
    class MockNetworkBus : public NetworkBus {
    public:
        void start(uint16_t port) override {
            std::cout << "[Network] Mock bus listening on port " << port << std::endl;
        }
        void stop() override {}
        void broadcast_state(const std::string& msg) override {
            std::cout << "[Network] Broadcasting: " << msg << std::endl;
        }
        void request_shard(const ShardID&, std::function<void(const std::vector<uint8_t>&)>) override {}
    };
}
