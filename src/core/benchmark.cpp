#include "benchmark.h"
#include <chrono>
#include <cmath>
#include <iostream>

namespace plexome {

    BenchmarkResult HardwareBenchmark::run_test() {
        std::cout << "[Benchmark] Testing hardware power (calculating parrots)..." << std::endl;

        auto start = std::chrono::high_resolution_clock::now();
        volatile double dummy = 0.0;
        uint64_t iterations = 0;

        // Simple 1-second math loop to measure pure CPU throughput
        while (std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::high_resolution_clock::now() - start).count() < 1) 
        {
            for (int i = 0; i < 1000000; ++i) {
                dummy += std::sqrt(static_cast<double>(i));
            }
            iterations++;
        }

        uint32_t score = static_cast<uint32_t>(iterations / 10); // Our "Parrots"
        BenchmarkResult res;
        res.score = score;

        if (score < 500) {
            res.tier = PerformanceTier::Potato;
            res.recommended_model = "Phi-3.5-mini (3.8B)";
        } else if (score < 2000) {
            res.tier = PerformanceTier::Standard;
            res.recommended_model = "Llama-3-8B-Instruct";
        } else {
            res.tier = PerformanceTier::Titan;
            res.recommended_model = "Mistral-Nemo-12B";
        }

        std::cout << "[Benchmark] Result: " << score << " PlexoParrots. Tier: " 
                  << (int)res.tier << " [Recommended: " << res.recommended_model << "]" << std::endl;
        
        return res;
    }
}
