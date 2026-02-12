#include "GeometryPass.h"
#include "RenderContext.h"
#include "RenderHelper/RenderHelper.h"
#include <glad/glad.h>

RENDERER_NAMESPACE_BEGIN

GeometryPass::GeometryPass(const int &width, const int &height, const std::shared_ptr<Shader> &shader)
    : m_shader(shader)
{
    m_positionTexture = RenderHelper::CreateTexture2D(width, height, GL_RGB32F, GL_RGB, GL_FLOAT);
    m_normalTexture = RenderHelper::CreateTexture2D(width, height, GL_RGB32F, GL_RGB, GL_FLOAT);
    m_diffuseTexture = RenderHelper::CreateTexture2D(width, height, GL_RGB32F, GL_RGB, GL_FLOAT);
    m_specularTexture = RenderHelper::CreateTexture2D(width, height, GL_RGB32F, GL_RGB, GL_FLOAT);
    m_shininessTexture = RenderHelper::CreateTexture2D(width, height, GL_R32F, GL_RED, GL_FLOAT);
    m_uidTexture = RenderHelper::CreateTexture2D(width, height, GL_R32I, GL_RED_INTEGER, GL_INT, GL_NEAREST);
    m_depthTexture = RenderHelper::CreateTexture2D(width, height, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT);

    m_frameBuffer.Attach(FrameBuffer::Attachment::Color0, m_positionTexture);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Color1, m_normalTexture);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Color2, m_diffuseTexture);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Color3, m_specularTexture);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Color4, m_shininessTexture);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Color5, m_uidTexture);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Depth, m_depthTexture);

    m_frameBuffer.Bind();
    GLenum drawBuffers[] = {
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5,
    };
    glDrawBuffers(6, drawBuffers);
    m_frameBuffer.Unbind();
}

GeometryPass::~GeometryPass()
{
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color0);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color1);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color2);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color3);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color4);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color5);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Depth);
    if (m_positionTexture != 0)
        glDeleteTextures(1, &m_positionTexture);
    if (m_normalTexture != 0)
        glDeleteTextures(1, &m_normalTexture);
    if (m_diffuseTexture != 0)
        glDeleteTextures(1, &m_diffuseTexture);
    if (m_specularTexture != 0)
        glDeleteTextures(1, &m_specularTexture);
    if (m_shininessTexture != 0)
        glDeleteTextures(1, &m_shininessTexture);
    if (m_uidTexture != 0)
        glDeleteTextures(1, &m_uidTexture);
    if (m_depthTexture != 0)
        glDeleteTextures(1, &m_depthTexture);
}

void GeometryPass::Execute(RenderContext &ctx)
{
    // ---- 绑定 FBO，清屏 ----
    m_frameBuffer.Bind();
    m_frameBuffer.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    m_frameBuffer.ClearDepthStencil(1.0f, 0);
    GLint clearValue[] = {-1};
    glClearBufferiv(GL_COLOR, 5, clearValue);

    // ---- 设置全局 uniform ----
    m_shader->use();
    m_shader->setMat4("view", ctx.viewMatrix);
    m_shader->setMat4("projection", ctx.projMatrix);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // ---- 遍历场景物体 ----
    if (ctx.sceneRenderables)
    {
        for (const auto &renderable : *ctx.sceneRenderables)
        {
            if (renderable)
                RenderRenderable(renderable.get());
        }
    }

    // ---- 解绑 ----
    m_shader->unuse();
    m_frameBuffer.Unbind();

    // ---- 将 G-Buffer 纹理写入上下文 ----
    ctx.gPositionTex = m_positionTexture;
    ctx.gNormalTex = m_normalTexture;
    ctx.gDiffuseTex = m_diffuseTexture;
    ctx.gSpecularTex = m_specularTexture;
    ctx.gShininessTex = m_shininessTexture;
    ctx.gUIDTex = m_uidTexture;
    ctx.gDepthTex = m_depthTexture;
}

void GeometryPass::Resize(int width, int height)
{
    auto resizeTex = [width, height](unsigned int tex, int internalFormat, int format, int type) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);
    };

    resizeTex(m_positionTexture, GL_RGB32F, GL_RGB, GL_FLOAT);
    resizeTex(m_normalTexture, GL_RGB32F, GL_RGB, GL_FLOAT);
    resizeTex(m_diffuseTexture, GL_RGB32F, GL_RGB, GL_FLOAT);
    resizeTex(m_specularTexture, GL_RGB32F, GL_RGB, GL_FLOAT);
    resizeTex(m_shininessTexture, GL_R32F, GL_RED, GL_FLOAT);
    resizeTex(m_uidTexture, GL_R32I, GL_RED_INTEGER, GL_INT);
    resizeTex(m_depthTexture, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GeometryPass::RenderRenderable(Renderable *renderable)
{
    const Mat4 &modelMatrix = renderable->m_transform.GetMatrix();
    m_shader->setMat4("model", modelMatrix.data());
    m_shader->setInt("uUID", static_cast<int>(renderable->getUid()));
    m_shader->setVec3("uColor", renderable->getColor().x, renderable->getColor().y, renderable->getColor().z);

    if (renderable->getType() == RenderableType::Primitive && renderable->getPrimitive())
    {
        if (renderable->getMaterial())
            renderable->getMaterial()->UpdateShaderParams(m_shader);
        renderable->getPrimitive()->draw();
    }
    else if (renderable->getType() == RenderableType::Model && renderable->getModel())
    {
        renderable->getModel()->draw(m_shader);
    }
}

int GeometryPass::GetCurrentSelectedUID(unsigned int mouseX, unsigned int mouseY)
{
    m_frameBuffer.Bind();
    glReadBuffer(GL_COLOR_ATTACHMENT5);
    int uid;
    glReadPixels(mouseX, mouseY, 1, 1, GL_RED_INTEGER, GL_INT, &uid);
    m_frameBuffer.Unbind();
    return uid;
}

RENDERER_NAMESPACE_END
