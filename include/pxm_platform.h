#ifndef PXM_PLATFORM_H
#define PXM_PLATFORM_H

// Detection of OS and Compiler for proper DLL export/import symbols
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    #define PXM_PLATFORM_WINDOWS
    #ifdef PXM_EXPORT
        #define PXM_API __declspec(dllexport)
    #else
        #define PXM_API __declspec(dllimport)
    #endif
#else
    #define PXM_PLATFORM_POSIX
    #define PXM_API __attribute__((visibility("default")))
#endif

// Helper for dynamic loading headers
#ifdef PXM_PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

#endif // PXM_PLATFORM_H
