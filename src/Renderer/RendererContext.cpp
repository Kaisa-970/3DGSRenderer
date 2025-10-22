#include "RendererContext.h"
#include <glad/glad.h>

RENDERER_NAMESPACE_BEGIN

RendererContext::RendererContext() {
    // 初始化代码可以放这里
    glEnable(GL_DEPTH_TEST);
}

RendererContext::~RendererContext() {
    // 清理代码
}

void RendererContext::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

RENDERER_NAMESPACE_END