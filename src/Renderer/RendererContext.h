#pragma once

#include "Core/RenderCore.h"
#include "Window.h"
#include "Shader.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API RendererContext {
public:
    RendererContext();
    ~RendererContext();

    // 删除拷贝
    RendererContext(const RendererContext&) = delete;
    RendererContext& operator=(const RendererContext&) = delete;

    // 清屏
    void clear(float r, float g, float b, float a = 1.0f);

    void viewPort(int x, int y, int width, int height);
};

RENDERER_NAMESPACE_END