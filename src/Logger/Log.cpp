#include "Log.h"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Logger {

std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

void Log::Init()
{
    // 设置日志格式: [时间戳] [日志名称] [日志级别] 消息
    spdlog::set_pattern("%^[%T] [%n] [%l] %v%$");
    
    // 创建核心日志器 (用于渲染器内部)
    s_CoreLogger = spdlog::stdout_color_mt("RENDERER");
    s_CoreLogger->set_level(spdlog::level::trace);
    
    // 创建客户端日志器 (用于应用程序)
    s_ClientLogger = spdlog::stdout_color_mt("APP");
    s_ClientLogger->set_level(spdlog::level::trace);
}

} // namespace Logger

