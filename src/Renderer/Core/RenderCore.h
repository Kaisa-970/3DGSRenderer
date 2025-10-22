#pragma once

#define RENDERER_NAMESPACE_BEGIN namespace Renderer {
#define RENDERER_NAMESPACE_END }

#if defined(RENDERER_EXPORTS)
    #if defined(GSRENDERER_OS_WINDOWS)
        #define RENDERER_API __declspec(dllexport)
    #elif defined(GSRENDERER_OS_MACOS) || defined(GSRENDERER_OS_LINUX)
        #define RENDERER_API __attribute__((visibility("default")))
    #endif
#else
    #if defined(GSRENDERER_OS_WINDOWS)
        #define RENDERER_API __declspec(dllimport)
    #elif defined(GSRENDERER_OS_MACOS) || defined(GSRENDERER_OS_LINUX)
        #define RENDERER_API 
    #endif
#endif