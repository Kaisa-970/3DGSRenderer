#include "PostProcessPass.h"
#include "RenderContext.h"
#include "RenderHelper/RenderHelper.h"
#include <glad/glad.h>

RENDERER_NAMESPACE_BEGIN

static const float vertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
                                 1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f};

PostProcessPass::PostProcessPass(const int &width, const int &height, const std::shared_ptr<Shader> &shader)
    : m_shader(shader)
{
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    m_colorTexture = RenderHelper::CreateTexture2D(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Color0, m_colorTexture);
}

PostProcessPass::~PostProcessPass()
{
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color0);
    if (m_colorTexture != 0)
        glDeleteTextures(1, &m_colorTexture);
    if (m_vao != 0)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0)
        glDeleteBuffers(1, &m_vbo);
}

void PostProcessPass::Execute(RenderContext &ctx)
{
    m_frameBuffer.Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // alpha=1 确保不透明
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_shader->use();

    // ---- 从 ctx 读取 G-Buffer + 光照纹理 ----
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ctx.gPositionTex);
    m_shader->setInt("u_positionTexture", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ctx.gNormalTex);
    m_shader->setInt("u_normalTexture", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ctx.lightingTex);
    m_shader->setInt("u_lightingTexture", 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, ctx.gDepthTex);
    m_shader->setInt("u_depthTexture", 3);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, ctx.gUIDTex);
    m_shader->setInt("u_uidTexture", 4);

    // ---- 从 ctx 读取相机位置和选中 UID ----
    if (ctx.camera)
    {
        m_shader->setVec3("viewPos", ctx.camera->getPosition().x, ctx.camera->getPosition().y,
                          ctx.camera->getPosition().z);
    }
    m_shader->setInt("u_currentSelectedUID", ctx.selectedUID);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    m_shader->unuse();
    m_frameBuffer.Unbind();

    // ---- 将后处理颜色纹理写入上下文 ----
    ctx.postProcessColorTex = m_colorTexture;
}

void PostProcessPass::Resize(int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

RENDERER_NAMESPACE_END
