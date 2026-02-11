#pragma once

#include "Core/RenderCore.h"

RENDERER_NAMESPACE_BEGIN

struct RenderContext;

/// 后处理效果基类
/// 每个效果实现 Apply()：从 inputTex 采样，绘制到当前绑定的 FBO
/// FBO 的绑定/切换由 PostProcessChain 管理，效果本身不管理 FBO
class RENDERER_API PostProcessEffect
{
public:
    virtual ~PostProcessEffect() = default;

    /// 执行效果：读 inputTex，画到当前绑定的 FBO
    /// @param inputTex  上一个效果的输出纹理（或初始的 lightingTex）
    /// @param quadVAO   共享的全屏四边形 VAO（由 PostProcessChain 提供）
    /// @param width     渲染宽度
    /// @param height    渲染高度
    /// @param ctx       渲染上下文（可访问 G-Buffer 等额外数据）
    virtual void Apply(unsigned int inputTex, unsigned int quadVAO,
                       int width, int height, const RenderContext &ctx) = 0;

    /// 当渲染尺寸变化时重建内部资源（默认无需处理）
    virtual void Resize(int width, int height) { (void)width; (void)height; }

    /// 返回效果名称
    virtual const char *GetName() const = 0;

    /// 运行时开关
    bool enabled = true;
};

RENDERER_NAMESPACE_END
