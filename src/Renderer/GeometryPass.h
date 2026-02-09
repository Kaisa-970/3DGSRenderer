#pragma once

#include "Core/RenderCore.h"
#include "FrameBuffer.h"
#include "Shader.h"
#include "Renderable.h"

RENDERER_NAMESPACE_BEGIN

struct RenderContext;

class RENDERER_API GeometryPass {
public:
    GeometryPass(const int& width, const int& height);
    ~GeometryPass();

    /// 统一执行接口：从 ctx 读取场景数据，将 G-Buffer 纹理 ID 写回 ctx
    void Execute(RenderContext& ctx);

    /// 物体拾取（读取 UID 纹理）
    unsigned int getCurrentSelectedUID(unsigned int mouseX, unsigned int mouseY);

private:
    /// 渲染单个 Renderable（设置 uniform + 绘制几何体）
    void RenderRenderable(Renderable* renderable);

    FrameBuffer m_frameBuffer;
    std::shared_ptr<Shader> m_shader;
    unsigned int m_positionTexture;
    unsigned int m_normalTexture;
    unsigned int m_diffuseTexture;
    unsigned int m_specularTexture;
    unsigned int m_shininessTexture;
    unsigned int m_uidTexture;
    unsigned int m_depthTexture;
};

RENDERER_NAMESPACE_END
