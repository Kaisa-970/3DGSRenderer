#pragma once

#include "Core/RenderCore.h"
#include "PostProcessEffect.h"
#include "FrameBuffer.h"
#include "Shader.h"
#include <memory>

RENDERER_NAMESPACE_BEGIN

/// Bloom（辉光）后处理效果
/// 内部多 pass：亮度提取 → 分离式高斯模糊 × N → 合成叠加
class RENDERER_API BloomEffect : public PostProcessEffect
{
public:
    BloomEffect(int width, int height,
                const std::shared_ptr<Shader> &thresholdShader,
                const std::shared_ptr<Shader> &blurShader,
                const std::shared_ptr<Shader> &compositeShader);
    ~BloomEffect() override;

    void Apply(unsigned int inputTex, unsigned int quadVAO,
               int width, int height, const RenderContext &ctx) override;
    void Resize(int width, int height) override;
    const char *GetName() const override { return "BloomEffect"; }

    // GUI 可调参数
    float threshold = 1.0f;    // 亮度阈值（> threshold 的部分才发光）
    float intensity = 0.5f;    // Bloom 叠加强度
    int blurIterations = 5;    // 高斯模糊迭代次数（越多越柔和）

private:
    std::shared_ptr<Shader> m_thresholdShader;
    std::shared_ptr<Shader> m_blurShader;
    std::shared_ptr<Shader> m_compositeShader;

    // 内部 FBO（Bloom 自己的工作空间，不占用链的 ping-pong）
    FrameBuffer m_fboThreshold;
    FrameBuffer m_fboBlurA;
    FrameBuffer m_fboBlurB;
    unsigned int m_texThreshold = 0;
    unsigned int m_texBlurA = 0;
    unsigned int m_texBlurB = 0;
};

RENDERER_NAMESPACE_END
