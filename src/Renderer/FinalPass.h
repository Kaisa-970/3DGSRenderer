#pragma once

#include "Core/RenderCore.h"
#include "Shader.h"

RENDERER_NAMESPACE_BEGIN

struct RenderContext;

class RENDERER_API FinalPass {
public:
    FinalPass();
    ~FinalPass();

    /// 统一执行接口：从 ctx 读取 displayTex，输出到屏幕
    void Execute(RenderContext& ctx);

private:
    std::shared_ptr<Shader> m_shader;
    unsigned int m_vao;
    unsigned int m_vbo;
};

RENDERER_NAMESPACE_END
