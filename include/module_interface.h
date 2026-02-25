#ifndef PXM_MODULE_INTERFACE_H
#define PXM_MODULE_INTERFACE_H

#ifdef _WIN32
    #define PXM_API extern "C" __declspec(dllexport)
#else
    #define PXM_API extern "C"
#endif

// Structure to identify the module
struct ModuleInfo {
    const char* name;
    const char* version;
    const char* description;
};

// Function pointer types for dynamic loading
typedef ModuleInfo (*PXM_GET_INFO_FUNC)();
typedef bool (*PXM_INIT_FUNC)();
typedef void (*PXM_SHUTDOWN_FUNC)();
typedef void (*PXM_RUN_FUNC)(); // Main logic loop of the module

#endif // PXM_MODULE_INTERFACE_H
