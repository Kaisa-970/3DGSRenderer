#pragma once

#define GSENGINE_NAMESPACE_BEGIN \
    namespace GSEngine           \
    {
#define GSENGINE_NAMESPACE_END }

#if defined(GSENGINE_EXPORTS)
#if defined(GSENGINE_OS_WINDOWS)
#define GSENGINE_API __declspec(dllexport)
#elif defined(GSENGINE_OS_MACOS) || defined(GSENGINE_OS_LINUX)
#define GSENGINE_API __attribute__((visibility("default")))
#endif
#else
#if defined(GSENGINE_OS_WINDOWS)
#define GSENGINE_API __declspec(dllimport)
#elif defined(GSENGINE_OS_MACOS) || defined(GSENGINE_OS_LINUX)
#define GSENGINE_API
#endif
#endif
