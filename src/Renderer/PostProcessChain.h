#pragma once

#include "Core/RenderCore.h"
#include "IRenderPass.h"
#include "FrameBuffer.h"
#include "PostProcessEffect.h"
#include <memory>
#include <vector>

RENDERER_NAMESPACE_BEGIN

struct RenderContext;

/// 后处理效果链（替代原来的 PostProcessPass）
/// 使用 Ping-Pong 双缓冲串联多个 PostProcessEffect
class RENDERER_API PostProcessChain : public IRenderPass
{
public:
    PostProcessChain(int width, int height);
    ~PostProcessChain() override;

    // 禁止拷贝（vector<unique_ptr> 不可拷贝，MSVC dllexport 要求显式声明）
    PostProcessChain(const PostProcessChain &) = delete;
    PostProcessChain &operator=(const PostProcessChain &) = delete;
    PostProcessChain(PostProcessChain &&) = default;
    PostProcessChain &operator=(PostProcessChain &&) = default;

    /// 执行所有启用的后处理效果，结果写入 ctx.postProcessColorTex
    void Execute(RenderContext &ctx) override;
    void Resize(int width, int height) override;
    const char *GetName() const override { return "PostProcessChain"; }

    /// 添加效果到链尾
    void AddEffect(std::unique_ptr<PostProcessEffect> effect);
    /// 按名称查找效果
    PostProcessEffect *GetEffect(const char *name) const;
    /// 按名称移除效果
    bool RemoveEffect(const char *name);
    /// 获取效果数量
    size_t GetEffectCount() const { return m_effects.size(); }

private:
    // Ping-Pong 双缓冲
    FrameBuffer m_fboA;
    FrameBuffer m_fboB;
    unsigned int m_texA = 0;
    unsigned int m_texB = 0;

    // 共享全屏四边形（所有效果复用）
    unsigned int m_quadVAO = 0;
    unsigned int m_quadVBO = 0;

    // 效果列表（按执行顺序）
    std::vector<std::unique_ptr<PostProcessEffect>> m_effects;
};

RENDERER_NAMESPACE_END
