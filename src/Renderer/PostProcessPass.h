#pragma once

#include "Core/RenderCore.h"
#include "IRenderPass.h"
#include "FrameBuffer.h"
#include "Shader.h"

RENDERER_NAMESPACE_BEGIN

struct RenderContext;

class RENDERER_API PostProcessPass : public IRenderPass
{
public:
    PostProcessPass(const int &width, const int &height, const std::shared_ptr<Shader>& shader);
    ~PostProcessPass() override;

    /// 统一执行接口：从 ctx 读取 G-Buffer + 光照纹理，将后处理颜色纹理写回 ctx
    void Execute(RenderContext& ctx) override;
    void Resize(int width, int height) override;
    const char* GetName() const override { return "PostProcessPass"; }

private:
    std::shared_ptr<Shader> m_shader;
    FrameBuffer m_frameBuffer;
    unsigned int m_colorTexture;
    unsigned int m_vao;
    unsigned int m_vbo;
};

RENDERER_NAMESPACE_END
