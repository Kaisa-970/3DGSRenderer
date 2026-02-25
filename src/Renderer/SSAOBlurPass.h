#pragma once

#include "Core/RenderCore.h"
#include "IRenderPass.h"
#include "Shader.h"
#include "FrameBuffer.h"

RENDERER_NAMESPACE_BEGIN

struct RenderContext;

/// SSAO 模糊 Pass：对 ctx.ssaoTex 做可分离高斯模糊，写回 ctx.ssaoTex，用于去除 SSAO 噪点
class RENDERER_API SSAOBlurPass : public IRenderPass
{
public:
    SSAOBlurPass(int width, int height, const std::shared_ptr<Shader> &blurShader);
    ~SSAOBlurPass() override;

    void Execute(RenderContext &ctx) override;
    void Resize(int width, int height) override;
    const char *GetName() const override { return "SSAOBlurPass"; }

private:
    std::shared_ptr<Shader> m_blurShader;
    FrameBuffer m_fboBlurA;
    FrameBuffer m_fboBlurB;
    unsigned int m_texBlurA = 0;
    unsigned int m_texBlurB = 0;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    int m_width = 0;
    int m_height = 0;
};

RENDERER_NAMESPACE_END
