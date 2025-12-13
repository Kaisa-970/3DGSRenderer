#include "PostProcessPass.h"
#include "RenderHelper/RenderHelper.h"
#include <glad/glad.h>


RENDERER_NAMESPACE_BEGIN

static const float vertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
                                 1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f};

PostProcessPass::PostProcessPass(const int &width, const int &height)
    : m_shader(Renderer::Shader::fromFiles("res/shaders/final.vs.glsl", "res/shaders/postprocess.fs.glsl"))
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

    m_colorTexture = RenderHelper::CreateTexture2D(width, height, GL_RGB32F, GL_RGB, GL_FLOAT);
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

void PostProcessPass::render(int width, int height, Camera &camera, const unsigned int currentSelectedUID,
                             const unsigned int &uidTexture, const unsigned int &positionTexture,
                             const unsigned int &normalTexture, const unsigned int &lightingTexture,
                             const unsigned int &depthTexture)
{
    m_frameBuffer.Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_shader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, positionTexture);
    m_shader->setInt("u_positionTexture", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    m_shader->setInt("u_normalTexture", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, lightingTexture);
    m_shader->setInt("u_lightingTexture", 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    m_shader->setInt("u_depthTexture", 3);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, uidTexture);
    m_shader->setInt("u_uidTexture", 4);

    m_shader->setVec3("viewPos", camera.getPosition().x, camera.getPosition().y, camera.getPosition().z);

    m_shader->setUint("u_currentSelectedUID", currentSelectedUID);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    m_shader->unuse();
    m_frameBuffer.Unbind();
}
RENDERER_NAMESPACE_END
