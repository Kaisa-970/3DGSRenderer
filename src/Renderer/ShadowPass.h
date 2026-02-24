#pragma once

#include "Core/RenderCore.h"
#include "IRenderPass.h"
#include "Shader.h"
#include "FrameBuffer.h"

RENDERER_NAMESPACE_BEGIN

struct RenderContext;

class RENDERER_API ShadowPass : public IRenderPass
{
public:
    ShadowPass(int shadowMapResolution, const std::shared_ptr<Shader> &shader);
    ~ShadowPass() override;

    /// 统一执行接口：从 ctx 读取 G-Buffer + 光源数据，将光照纹理 ID 写回 ctx
    void Execute(RenderContext &ctx) override;
    void Resize(int width, int height) override;
    const char *GetName() const override
    {
        return "ShadowPass";
    }

private:
    std::shared_ptr<Shader> m_shader;
    FrameBuffer m_frameBuffer;
    unsigned int m_lightDepthTexture;
    int m_shadowMapResolution = 1024;
};

RENDERER_NAMESPACE_END
