#ifndef PXM_TYPES_H
#define PXM_TYPES_H

#include <cstdint>

/**
 * Performance levels assigned after benchmarking
 */
enum class PerformanceTier {
    POTATO = 0,    // Limited resources, scanner only
    STANDARD = 1,  // Normal peer
    TITAN = 2      // Queen node, consensus participant
};

/**
 * Global configuration shared with modules during initialization
 */
struct PxmConfig {
    bool is_seed;
    PerformanceTier tier;
    uint16_t network_port;
    const char* model_path;
    const char* work_dir;
};

#endif // PXM_TYPES_H
