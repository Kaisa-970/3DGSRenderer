#include "RendererContext.h"
#include "Logger/Log.h"
#include <glad/glad.h>

RENDERER_NAMESPACE_BEGIN

RendererContext::RendererContext()
{
    // 初始化代码可以放这里
    glEnable(GL_DEPTH_TEST);
}

RendererContext::~RendererContext()
{
    // 清理代码
}

void RendererContext::Init(void *(*getProcAddress)(const char *))
{
// 初始化 GLAD
#ifdef USE_GLES3
    if (!gladLoadGLES2Loader((GLADloadproc)getProcAddress))
    {
        throw std::runtime_error("Failed to initialize GLAD (OpenGL ES)");
    }
#else
    if (!gladLoadGLLoader((GLADloadproc)getProcAddress))
    {
        throw std::runtime_error("Failed to initialize GLAD (OpenGL)");
    }
#endif

// 打印 OpenGL 信息
#ifdef USE_GLES3
    std::cout << "=== OpenGL ES 3 Information ===" << std::endl;
#else
    LOG_CORE_INFO("=== OpenGL Information ===");
#endif
    LOG_CORE_INFO("Vendor: {}", (const char *)glGetString(GL_VENDOR));
    LOG_CORE_INFO("Renderer: {}", (const char *)glGetString(GL_RENDERER));
    LOG_CORE_INFO("Version: {}", (const char *)glGetString(GL_VERSION));
    LOG_CORE_INFO("GLSL Version: {}", (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
    LOG_CORE_INFO("================================");
}

void RendererContext::Shutdown()
{
}

void RendererContext::clear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

RENDERER_NAMESPACE_END
