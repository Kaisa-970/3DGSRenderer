#pragma once

#include "Core/RenderCore.h"
#include "Shader.h"
#include "FrameBuffer.h"

RENDERER_NAMESPACE_BEGIN

struct RenderContext;

class RENDERER_API LightingPass {
public:
    LightingPass(const int& width, const int& height);
    ~LightingPass();

    /// 统一执行接口：从 ctx 读取 G-Buffer + 光源数据，将光照纹理 ID 写回 ctx
    void Execute(RenderContext& ctx);

private:
    std::shared_ptr<Shader> m_shader;
    FrameBuffer m_frameBuffer;
    unsigned int m_lightingTexture;
    unsigned int m_vao;
    unsigned int m_vbo;
};

RENDERER_NAMESPACE_END
