#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #ifdef GAUSSIAN_RENDERER_EXPORTS
        #define GAUSSIAN_RENDERER_API __declspec(dllexport)
    #else
        #define GAUSSIAN_RENDERER_API __declspec(dllimport)
    #endif
#else
    #ifdef GAUSSIAN_RENDERER_EXPORTS
        #define GAUSSIAN_RENDERER_API __attribute__((visibility("default")))
    #else
        #define GAUSSIAN_RENDERER_API
    #endif
#endif

#define GAUSSIAN_RENDERER_NAMESPACE_BEGIN namespace GaussianRenderer {
#define GAUSSIAN_RENDERER_NAMESPACE_END }
