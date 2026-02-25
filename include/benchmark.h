#pragma once
#include "plexome_types.h"
#include <cstdint>
#include <string>

namespace plexome {

    // PerformanceTier is now defined in plexome_types.h

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
