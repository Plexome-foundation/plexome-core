#pragma once
#include "plexome_types.h"
#include <map>
#include <string>
#include <vector>

namespace plexome {

    struct ModelSlice {
        uint32_t start_layer;
        uint32_t end_layer;
        PeerID node_id;
        PerformanceTier tier;
    };

    /**
     * Directs the flow of tensors (neural network state) between nodes.
     */
    class SwarmRouter {
    public:
        // Builds the pipeline map based on active peers and their RAM/VRAM
        void rebuild_routing_table(const std::string& model_name, uint32_t total_layers);

        // Finds the node responsible for the NEXT slice of computation
        PeerID get_next_hop(uint32_t current_layer) const;

        // Serializes and sends a tensor array to the next node in the pipeline
        bool forward_tensor(PeerID target, const std::vector<float>& tensor_data);

        // Receives a tensor from the PREVIOUS node to continue computation
        std::vector<float> await_incoming_tensor();

    private:
        std::vector<ModelSlice> pipeline_map_;
    };
}
