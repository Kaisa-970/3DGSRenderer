#pragma once

#include "Core/RenderCore.h"
#include "FrameBuffer.h"

RENDERER_NAMESPACE_BEGIN

struct RenderContext;

class RENDERER_API ForwardPass
{
public:
    ForwardPass();
    ~ForwardPass() = default;

    /// 统一执行接口：从 ctx 读取前向渲染物体和纹理，渲染到 postProcessColorTex 上
    void Execute(RenderContext& ctx);

private:
    FrameBuffer m_frameBuffer;
};

RENDERER_NAMESPACE_END
