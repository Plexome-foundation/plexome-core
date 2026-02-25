#ifndef PXM_MODULE_INTERFACE_H
#define PXM_MODULE_INTERFACE_H

#ifdef _WIN32
    #define PXM_API extern "C" __declspec(dllexport)
#else
    #define PXM_API extern "C"
#endif

struct ModuleInfo {
    const char* name;
    const char* version;
};

// PXM_INIT_FUNC now accepts is_seed to configure the module role
typedef bool (*PXM_INIT_FUNC)(bool is_seed); 
typedef void (*PXM_SHUTDOWN_FUNC)();
typedef ModuleInfo (*PXM_GET_INFO_FUNC)();

#endif
