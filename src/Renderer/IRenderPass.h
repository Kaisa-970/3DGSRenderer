#pragma once

#include "Core/RenderCore.h"

RENDERER_NAMESPACE_BEGIN

struct RenderContext;

/// 渲染 Pass 的抽象基类
/// 所有 Pass 都实现此接口，由 RenderPipeline 统一驱动执行
class RENDERER_API IRenderPass
{
public:
    virtual ~IRenderPass() = default;

    /// 执行该 Pass：从 ctx 读取输入数据，将输出写回 ctx
    virtual void Execute(RenderContext& ctx) = 0;

    /// 返回 Pass 名称（用于调试/日志）
    virtual const char* GetName() const = 0;
};

RENDERER_NAMESPACE_END
