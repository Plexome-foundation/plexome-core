#ifndef PXM_MODULE_INTERFACE_H
#define PXM_MODULE_INTERFACE_H

#ifdef _WIN32
    #define PXM_API extern "C" __declspec(dllexport)
#else
    #define PXM_API extern "C"
#endif

// Every module must implement these basic functions
struct ModuleInfo {
    const char* name;
    const char* version;
    const char* description;
};

typedef void (*PXM_INIT_FUNC)();
typedef void (*PXM_SHUTDOWN_FUNC)();
typedef ModuleInfo (*PXM_GET_INFO_FUNC)();

#endif // PXM_MODULE_INTERFACE_H
