#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace plexome {

    enum class PerformanceTier {
        Potato,   // Low-end VPS, older CPUs
        Standard, // Modern PC, mid-range laptops
        Titan     // Servers with high-core CPUs or High-end GPUs
    };

    struct BenchmarkResult {
        uint32_t score; // Those "Parrots" (PlexoFlops)
        PerformanceTier tier;
        std::string recommended_model;
    };

    class HardwareBenchmark {
    public:
        // Runs a 2-second math stress test to evaluate the node
        static BenchmarkResult run_test();
    };
}
