#include "FinalPass.h"
#include "RenderContext.h"
#include "RenderHelper/RenderHelper.h"
#include <glad/glad.h>

RENDERER_NAMESPACE_BEGIN

static const float vertices[] = {
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f,
    -1.0f,  1.0f, 0.0f, 1.0f
};

FinalPass::FinalPass(int width, int height, const std::shared_ptr<Shader> &shader)
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
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // 输出纹理使用 GL_RGBA8（经过 tone mapping + gamma 后已是 LDR [0,1]）
    m_outputTexture = RenderHelper::CreateTexture2D(width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Color0, m_outputTexture);
}

FinalPass::~FinalPass()
{
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color0);
    if (m_outputTexture != 0)
        glDeleteTextures(1, &m_outputTexture);
    if (m_vao != 0)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0)
        glDeleteBuffers(1, &m_vbo);
}

void FinalPass::Execute(RenderContext &ctx)
{
    // 渲染到离屏 FBO（而非默认帧缓冲）
    m_frameBuffer.Bind();
    glViewport(0, 0, ctx.width, ctx.height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    m_shader->use();

    // 传入 HDR / Tone Mapping 参数
    m_shader->setFloat("u_exposure", ctx.exposure);
    m_shader->setInt("u_tonemapMode", ctx.tonemapMode);
    m_shader->setInt("u_displaySingleChannelR", ctx.displaySingleChannelR ? 1 : 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ctx.displayTex);
    m_shader->setInt("u_colorTexture", 0);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    m_shader->unuse();
    m_frameBuffer.Unbind();

    // 将 tone-mapped 的 LDR 纹理写回 ctx
    ctx.finalTex = m_outputTexture;
}

void FinalPass::Resize(int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, m_outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void FinalPass::PresentToScreen(int width, int height)
{
    // 将离屏纹理绘制到默认帧缓冲（屏幕）
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    m_shader->use();
    // 直接输出，不再做 tone mapping（已经在 Execute 中做过了）
    m_shader->setFloat("u_exposure", 1.0f);
    m_shader->setInt("u_tonemapMode", 0); // None

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_outputTexture);
    m_shader->setInt("u_colorTexture", 0);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    m_shader->unuse();
}

RENDERER_NAMESPACE_END
