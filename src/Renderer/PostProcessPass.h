#pragma once

#include "Core/RenderCore.h"
#include "FrameBuffer.h"
#include "Shader.h"

RENDERER_NAMESPACE_BEGIN

struct RenderContext;

class RENDERER_API PostProcessPass
{
public:
    PostProcessPass(const int &width, const int &height);
    ~PostProcessPass();

    /// 统一执行接口：从 ctx 读取 G-Buffer + 光照纹理，将后处理颜色纹理写回 ctx
    void Execute(RenderContext& ctx);

private:
    std::shared_ptr<Shader> m_shader;
    FrameBuffer m_frameBuffer;
    unsigned int m_colorTexture;
    unsigned int m_vao;
    unsigned int m_vbo;
};

RENDERER_NAMESPACE_END
