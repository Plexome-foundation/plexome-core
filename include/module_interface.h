#ifndef PXM_MODULE_INTERFACE_H
#define PXM_MODULE_INTERFACE_H

#include "plexome_types.h"
#include "pxm_platform.h"

/**
 * Common return codes for all Plexome modules
 */
enum class PxmStatus {
    OK = 0,
    ERROR_INIT_FAILED = 1,
    ERROR_MODULE_NOT_FOUND = 2,
    ERROR_INCOMPATIBLE_VERSION = 3
};

/**
 * Module information structure for the Host
 */
struct PxmModuleInfo {
    const char* name;
    const char* version;
    const char* description;
};

// Standard function pointer types for dynamic loading
typedef PxmModuleInfo (*PXM_GET_INFO_FUNC)();
typedef PxmStatus (*PXM_INIT_FUNC)(const PxmConfig* config);
typedef void (*PXM_SHUTDOWN_FUNC)();

#endif // PXM_MODULE_INTERFACE_H
