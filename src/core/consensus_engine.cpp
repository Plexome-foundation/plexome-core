#include "consensus_engine.h"
#include <iostream>
#include <algorithm>

namespace plexome {

    void ConsensusEngine::submit_result(const std::string& task_id, const std::vector<float>& result) {
        auto& task_votes = votes_[task_id];

        // Check if this specific result was already submitted
        auto it = std::find_if(task_votes.begin(), task_votes.end(), [&](const VoteResult& r) {
            return r.data == result;
        });

        if (it != task_votes.end()) {
            it->vote_count++;
        } else {
            task_votes.push_back({result, 1});
        }
        
        std::cout << "[Consensus] Vote registered for task: " << task_id << std::endl;
    }

    bool ConsensusEngine::has_consensus(const std::string& task_id, uint32_t total_expected_votes) {
        if (votes_.find(task_id) == votes_.end()) return false;

        for (const auto& r : votes_[task_id]) {
            if (r.vote_count > total_expected_votes / 2) {
                return true;
            }
        }
        return false;
    }

    std::vector<float> ConsensusEngine::get_winner(const std::string& task_id) {
        auto& task_votes = votes_[task_id];
        auto winner = std::max_element(task_votes.begin(), task_votes.end(), [](const VoteResult& a, const VoteResult& b) {
            return a.vote_count < b.vote_count;
        });
        return winner->data;
    }
}
