#pragma once

#include "Core/RenderCore.h"
#include "IRenderPass.h"
#include "FrameBuffer.h"
#include "Shader.h"

RENDERER_NAMESPACE_BEGIN

struct RenderContext;

class RENDERER_API FinalPass : public IRenderPass
{
public:
    FinalPass(int width, int height, const std::shared_ptr<Shader> &shader);
    ~FinalPass() override;

    /// 统一执行接口：从 ctx 读取 displayTex，做 ToneMapping + Gamma，输出到离屏纹理
    void Execute(RenderContext &ctx) override;
    void Resize(int width, int height) override;
    const char *GetName() const override { return "FinalPass"; }

    /// 将 FinalPass 的输出纹理 blit 到默认帧缓冲（屏幕）
    void PresentToScreen(int width, int height);

private:
    std::shared_ptr<Shader> m_shader;
    FrameBuffer m_frameBuffer;
    unsigned int m_outputTexture;
    unsigned int m_vao;
    unsigned int m_vbo;
};

RENDERER_NAMESPACE_END
