#pragma once

#include <spdlog/spdlog.h>
#include <memory>

// Logger模块的导出宏
#if defined(_WIN32) || defined(_WIN64)
    #ifdef LOGGER_EXPORTS
        #define LOGGER_API __declspec(dllexport)
    #else
        #define LOGGER_API __declspec(dllimport)
    #endif
#else
    #ifdef LOGGER_EXPORTS
        #define LOGGER_API __attribute__((visibility("default")))
    #else
        #define LOGGER_API
    #endif
#endif

namespace Logger {

class LOGGER_API Log
{
public:
    static void Init();
    
    inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
    inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

private:
    static std::shared_ptr<spdlog::logger> s_CoreLogger;
    static std::shared_ptr<spdlog::logger> s_ClientLogger;
};

} // namespace Logger

// Core log macros (用于库内部)
#define LOG_CORE_TRACE(...)    ::Logger::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define LOG_CORE_INFO(...)     ::Logger::Log::GetCoreLogger()->info(__VA_ARGS__)
#define LOG_CORE_WARN(...)     ::Logger::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define LOG_CORE_ERROR(...)    ::Logger::Log::GetCoreLogger()->error(__VA_ARGS__)
#define LOG_CORE_CRITICAL(...) ::Logger::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros (用于应用程序)
#define LOG_TRACE(...)         ::Logger::Log::GetClientLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...)          ::Logger::Log::GetClientLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)          ::Logger::Log::GetClientLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)         ::Logger::Log::GetClientLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...)      ::Logger::Log::GetClientLogger()->critical(__VA_ARGS__)

