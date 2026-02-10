#include "LightingPass.h"
#include "RenderContext.h"
#include "RenderHelper/RenderHelper.h"
#include <glad/glad.h>

RENDERER_NAMESPACE_BEGIN

static const float vertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
                                 1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f};

LightingPass::LightingPass(const int &width, const int &height, const std::shared_ptr<Shader> &shader)
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

    m_lightingTexture = RenderHelper::CreateTexture2D(width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Color0, m_lightingTexture);
}

LightingPass::~LightingPass()
{
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color0);
    if (m_lightingTexture != 0)
        glDeleteTextures(1, &m_lightingTexture);
    if (m_vao != 0)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0)
        glDeleteBuffers(1, &m_vbo);
}

void LightingPass::Execute(RenderContext &ctx)
{
    m_frameBuffer.Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_shader->use();

    // ---- 从 ctx 读取光源属性 ----
    if (ctx.light)
    {
        m_shader->setVec3("lightPos", ctx.light->position.x, ctx.light->position.y, ctx.light->position.z);
        m_shader->setVec3("lightColor", ctx.light->color.x * ctx.light->intensity,
                          ctx.light->color.y * ctx.light->intensity, ctx.light->color.z * ctx.light->intensity);
        m_shader->setFloat("ambientStrength", ctx.light->ambientStrength);
        m_shader->setFloat("diffuseStrength", ctx.light->diffuseStrength);
        m_shader->setFloat("specularStrength", ctx.light->specularStrength);
    }

    // ---- 从 ctx 读取相机位置 ----
    if (ctx.camera)
    {
        m_shader->setVec3("viewPos", ctx.camera->getPosition().x, ctx.camera->getPosition().y,
                          ctx.camera->getPosition().z);
    }

    // ---- 从 ctx 读取 G-Buffer 纹理 ----
    m_shader->setInt("shininess", 32);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ctx.gPositionTex);
    m_shader->setInt("u_positionTexture", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ctx.gNormalTex);
    m_shader->setInt("u_normalTexture", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ctx.gDiffuseTex);
    m_shader->setInt("u_diffuseTexture", 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, ctx.gSpecularTex);
    m_shader->setInt("u_specularTexture", 3);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, ctx.gShininessTex);
    m_shader->setInt("u_shininessTexture", 4);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    m_shader->unuse();
    m_frameBuffer.Unbind();

    // ---- 将光照纹理写入上下文 ----
    ctx.lightingTex = m_lightingTexture;
}

RENDERER_NAMESPACE_END
