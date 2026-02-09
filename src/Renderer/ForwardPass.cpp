#include "ForwardPass.h"
#include "RenderContext.h"
#include <glad/glad.h>

RENDERER_NAMESPACE_BEGIN

ForwardPass::ForwardPass()
{
}

void ForwardPass::Execute(RenderContext& ctx)
{
    if (!ctx.forwardRenderables || ctx.forwardRenderables->empty() || !ctx.forwardShader)
        return;

    // 将颜色写入后处理阶段的颜色纹理，同时复用几何阶段的深度纹理以保持遮挡关系
    m_frameBuffer.Attach(FrameBuffer::Attachment::Color0, ctx.postProcessColorTex);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Depth, ctx.gDepthTex);
    m_frameBuffer.Bind();

    glViewport(0, 0, ctx.width, ctx.height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    auto& shader = ctx.forwardShader;
    shader->use();
    shader->setMat4("view", ctx.viewMatrix);
    shader->setMat4("projection", ctx.projMatrix);
    shader->setFloat("u_time", ctx.currentTime);

    // 设置相机位置
    if (ctx.camera)
    {
        float vx, vy, vz;
        ctx.camera->getPosition(vx, vy, vz);
        shader->setVec3("u_viewPos", vx, vy, vz);
    }

    for (const auto &r : *ctx.forwardRenderables)
    {
        if (!r)
            continue;

        shader->setMat4("model", r->getTransform().m);
        shader->setInt("uUID", static_cast<int>(r->getUid()));
        shader->setVec3("uColor", r->getColor().x, r->getColor().y, r->getColor().z);

        if (r->getType() == RenderableType::Primitive && r->getPrimitive())
        {
            if (r->getMaterial())
                r->getMaterial()->UpdateShaderParams(shader);
            r->getPrimitive()->draw();
        }
        else if (r->getType() == RenderableType::Model && r->getModel())
        {
            r->getModel()->draw(shader);
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    shader->unuse();
    m_frameBuffer.Unbind();
}

RENDERER_NAMESPACE_END
