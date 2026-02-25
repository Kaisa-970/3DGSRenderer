#include "ShadowPass.h"
#include "RenderContext.h"
#include "RenderHelper/RenderHelper.h"
#include "Light.h"
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
    // 阴影贴图边缘采样用 1.0（表示“无遮挡”），避免光锥外错误阴影
    glBindTexture(GL_TEXTURE_2D, m_lightDepthTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);
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

    // 无光源或第一盏不是平行光时，不渲染阴影（深度贴图保持清除后的状态，采样结果为“无遮挡”）
    if (!ctx.lights || ctx.lights->empty())
    {
        m_frameBuffer.Unbind();
        glViewport(0, 0, ctx.width, ctx.height);
        ctx.shadowTex = m_lightDepthTexture;
        return;
    }

    const auto &firstLight = ctx.lights->at(0);
    if (firstLight->type != LightType::Directional)
    {
        m_frameBuffer.Unbind();
        glViewport(0, 0, ctx.width, ctx.height);
        ctx.shadowTex = m_lightDepthTexture;
        return;
    }

    m_shader->use();
    m_shader->setMat4("projViewMat", firstLight->GetViewProjectionMatrix().data());

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
