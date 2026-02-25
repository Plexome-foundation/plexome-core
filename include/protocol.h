#pragma once
#include "plexome_types.h"
#include <vector>
#include <string>

namespace plexome {

    const uint32_t PROTOCOL_VERSION = 1;

    /**
     * Handshake packet sent immediately after connection is established.
     * Ensures compatibility and identity verification.
     */
    struct HandshakePacket {
        uint32_t version;
        PeerID sender_id;
        NodeRole sender_role;
        uint64_t vram_available;
        // Reserved for future crypto signature (Ed25519)
        std::vector<uint8_t> signature; 

        // Serialize to binary buffer (simple placeholder)
        std::vector<uint8_t> serialize() const;
        static HandshakePacket deserialize(const std::vector<uint8_t>& buffer);
    };

    /**
     * Types of messages exchanged on the Plexome Storage Bus
     */
    enum class MessageType : uint8_t {
        Handshake = 0x01,
        ShardRequest = 0x02,
        ShardResponse = 0x03,
        TaskAnnouncement = 0x04,
        ResultSubmission = 0x05,
        Heartbeat = 0x06
    };
}
