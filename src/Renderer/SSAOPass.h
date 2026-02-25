#pragma once

#include "Core/RenderCore.h"
#include "IRenderPass.h"
#include "Shader.h"
#include "FrameBuffer.h"

RENDERER_NAMESPACE_BEGIN

struct RenderContext;

/// SSAO Pass：从 G-Buffer 的 position/normal 计算环境光遮蔽，输出单通道 AO 纹理到 ctx.ssaoTex
class RENDERER_API SSAOPass : public IRenderPass
{
public:
    SSAOPass(int width, int height, const std::shared_ptr<Shader> &shader);
    ~SSAOPass() override;

    void Execute(RenderContext &ctx) override;
    void Resize(int width, int height) override;
    const char *GetName() const override { return "SSAOPass"; }

private:
    void initNoiseTexture();
    void initKernel();

    std::shared_ptr<Shader> m_shader;
    FrameBuffer m_frameBuffer;
    unsigned int m_aoTexture = 0;
    unsigned int m_noiseTexture = 0;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    int m_width = 0;
    int m_height = 0;
    static const int KERNEL_SIZE = 16;
    float m_kernel[KERNEL_SIZE * 3] = {};
};

RENDERER_NAMESPACE_END
