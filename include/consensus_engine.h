#pragma once
#include "plexome_types.h"
#include <map>
#include <string>
#include <vector>

namespace plexome {

    /**
     * Implements Majority Vote validation.
     * Ensures that computation results are consistent across the swarm.
     */
    class ConsensusEngine {
    public:
        struct VoteResult {
            std::vector<float> data;
            uint32_t vote_count;
        };

        // Submit a result for a specific task from a peer
        void submit_result(const std::string& task_id, const std::vector<float>& result);

        // Check if we have reached a majority consensus ( > 50% )
        bool has_consensus(const std::string& task_id, uint32_t total_expected_votes);

        // Get the winning result
        std::vector<float> get_winner(const std::string& task_id);

    private:
        // TaskID -> List of unique results and their counts
        std::map<std::string, std::vector<VoteResult>> votes_;
    };
}
