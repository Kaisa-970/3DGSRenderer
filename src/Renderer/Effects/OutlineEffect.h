#pragma once

#include "Core/RenderCore.h"
#include "PostProcessEffect.h"
#include "Shader.h"
#include <memory>

RENDERER_NAMESPACE_BEGIN

/// 选中物体描边效果
/// 使用 Sobel 深度边缘 + Roberts Cross 法线边缘检测
class RENDERER_API OutlineEffect : public PostProcessEffect
{
public:
    explicit OutlineEffect(const std::shared_ptr<Shader> &shader);
    ~OutlineEffect() override = default;

    void Apply(unsigned int inputTex, unsigned int quadVAO,
               int width, int height, const RenderContext &ctx) override;
    const char *GetName() const override { return "OutlineEffect"; }

private:
    std::shared_ptr<Shader> m_shader;
};

RENDERER_NAMESPACE_END
