#include "ForwardPass.h"
#include <glad/glad.h>
RENDERER_NAMESPACE_BEGIN

ForwardPass::ForwardPass()
{
}

void ForwardPass::Render(int width, int height, const float *view, const float *projection, unsigned int colorTexture,
                         unsigned int depthTexture, const std::vector<std::shared_ptr<Renderable>> &renderables,
                         std::shared_ptr<Shader> shader, float timeSeconds)
{
    // 将颜色写入后处理阶段的颜色纹理，同时复用几何阶段的深度纹理以保持遮挡关系
    m_frameBuffer.Attach(FrameBuffer::Attachment::Color0, colorTexture);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Depth, depthTexture);
    m_frameBuffer.Bind();

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // 半透明通常不写入深度，避免挡住后续片元
    glDisable(GL_CULL_FACE);

    shader->use();
    shader->setMat4("view", view);
    shader->setMat4("projection", projection);
    shader->setFloat("u_time", timeSeconds);
    for (const auto &r : renderables)
    {
        if (!r)
            continue;

        // 由 ForwardPass 负责设置 per-object uniform
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
