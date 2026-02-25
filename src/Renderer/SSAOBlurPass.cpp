#include "SSAOBlurPass.h"
#include "RenderContext.h"
#include "RenderHelper/RenderHelper.h"
#include <glad/glad.h>

RENDERER_NAMESPACE_BEGIN

static const float s_quadVertices[] = {
    -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,
    1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f};

SSAOBlurPass::SSAOBlurPass(int width, int height, const std::shared_ptr<Shader> &blurShader)
    : m_blurShader(blurShader), m_width(width), m_height(height)
{
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_quadVertices), s_quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_texBlurA = RenderHelper::CreateTexture2D(width, height, GL_R32F, GL_RED, GL_FLOAT);
    m_texBlurB = RenderHelper::CreateTexture2D(width, height, GL_R32F, GL_RED, GL_FLOAT);
    m_fboBlurA.Attach(FrameBuffer::Attachment::Color0, m_texBlurA);
    m_fboBlurB.Attach(FrameBuffer::Attachment::Color0, m_texBlurB);
}

SSAOBlurPass::~SSAOBlurPass()
{
    m_fboBlurA.Detach(FrameBuffer::Attachment::Color0);
    m_fboBlurB.Detach(FrameBuffer::Attachment::Color0);
    if (m_texBlurA != 0)
        glDeleteTextures(1, &m_texBlurA);
    if (m_texBlurB != 0)
        glDeleteTextures(1, &m_texBlurB);
    if (m_vao != 0)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0)
        glDeleteBuffers(1, &m_vbo);
}

void SSAOBlurPass::Execute(RenderContext &ctx)
{
    if (ctx.ssaoTex == 0)
        return;

    glDisable(GL_DEPTH_TEST);

    m_blurShader->use();

    // 水平模糊：ctx.ssaoTex -> m_texBlurA
    m_fboBlurA.Bind();
    glViewport(0, 0, ctx.width, ctx.height);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    m_blurShader->setInt("u_horizontal", 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ctx.ssaoTex);
    m_blurShader->setInt("u_inputTexture", 0);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    // 垂直模糊：m_texBlurA -> m_texBlurB
    m_fboBlurB.Bind();
    glClear(GL_COLOR_BUFFER_BIT);
    m_blurShader->setInt("u_horizontal", 0);
    glBindTexture(GL_TEXTURE_2D, m_texBlurA);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    m_blurShader->unuse();
    m_fboBlurB.Unbind();

    ctx.ssaoTex = m_texBlurB;
}

void SSAOBlurPass::Resize(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;
    m_width = width;
    m_height = height;
    glBindTexture(GL_TEXTURE_2D, m_texBlurA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, m_texBlurB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

RENDERER_NAMESPACE_END
