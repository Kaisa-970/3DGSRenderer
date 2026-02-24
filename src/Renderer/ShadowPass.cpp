#include "ShadowPass.h"
#include "RenderContext.h"
#include "RenderHelper/RenderHelper.h"
#include <glad/glad.h>

RENDERER_NAMESPACE_BEGIN

ShadowPass::ShadowPass(int shadowMapResolution, const std::shared_ptr<Shader> &shader)
    : m_shader(shader), m_shadowMapResolution(shadowMapResolution)
{
    m_frameBuffer.Bind();
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    m_lightDepthTexture = RenderHelper::CreateTexture2D(shadowMapResolution, shadowMapResolution, GL_DEPTH_COMPONENT24,
                                                        GL_DEPTH_COMPONENT, GL_FLOAT);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Depth, m_lightDepthTexture);
}

ShadowPass::~ShadowPass()
{
    m_frameBuffer.Detach(FrameBuffer::Attachment::Depth);
    if (m_lightDepthTexture != 0)
        glDeleteTextures(1, &m_lightDepthTexture);
}

void ShadowPass::Execute(RenderContext &ctx)
{
    glViewport(0, 0, m_shadowMapResolution, m_shadowMapResolution);
    m_frameBuffer.Bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    m_shader->use();

    // ---- 从 ctx 读取光源 ----
    if (ctx.lights->size() > 0)
    {
        auto directionalLight = ctx.lights->at(0);
        m_shader->setMat4("projViewMat", directionalLight->GetViewProjectionMatrix().data());
    }

    // ---- 遍历场景物体 ----
    if (ctx.sceneRenderables)
    {
        for (const auto &renderable : *ctx.sceneRenderables)
        {
            if (renderable)
            {
                const Mat4 &model = renderable->m_transform.GetMatrix();
                m_shader->setMat4("modelMat", model.data());
                if (renderable->getType() == RenderableType::Primitive && renderable->getPrimitive())
                {
                    renderable->getPrimitive()->draw();
                }
                else if (renderable->getType() == RenderableType::Model && renderable->getModel())
                {
                    renderable->getModel()->draw(m_shader);
                }
            }
        }
    }

    m_shader->unuse();
    m_frameBuffer.Unbind();
    glViewport(0, 0, ctx.width, ctx.height);
    // ---- 将光照纹理写入上下文 ----
    ctx.shadowTex = m_lightDepthTexture;
}

void ShadowPass::Resize(int width, int height)
{
    // glBindTexture(GL_TEXTURE_2D, m_lightDepthTexture);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    // glBindTexture(GL_TEXTURE_2D, 0);
}

RENDERER_NAMESPACE_END
