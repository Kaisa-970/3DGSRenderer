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

    m_lightingTexture = RenderHelper::CreateTexture2D(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT);
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
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // alpha=1 确保背景不透明，避免 ImGui 显示时透明穿透
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_shader->use();

    // ---- 从 ctx 读取光源属性 ----
    m_shader->setInt("numLights", ctx.lights->size());
    if (ctx.lights && !ctx.lights->empty())
    {
        auto directionalLight = ctx.lights->at(0);
        m_shader->setMat4("lightSpaceMat", directionalLight->GetViewProjectionMatrix().data());
        m_shader->setVec3("directionalLightDirection", directionalLight->direction.x, directionalLight->direction.y,
                          directionalLight->direction.z);

        for (int i = 0; i < ctx.lights->size(); i++)
        {
            const std::shared_ptr<Light> light = ctx.lights->at(i);
            std::string lightName = "lights[" + std::to_string(i) + "]";
            m_shader->setVec3((lightName + ".position").c_str(), light->position.x, light->position.y,
                              light->position.z);
            m_shader->setVec3((lightName + ".color").c_str(), light->color.x, light->color.y, light->color.z);
            m_shader->setFloat((lightName + ".intensity").c_str(), light->intensity);
        }
    }
    m_shader->setFloat("ambientStrength", ctx.ambientStrength);
    m_shader->setFloat("diffuseStrength", ctx.diffuseStrength);
    m_shader->setFloat("specularStrength", ctx.specularStrength);

    // ---- 从 ctx 读取相机位置 ----
    if (ctx.camera)
    {
        m_shader->setVec3("viewPos", ctx.camera->getPosition().x, ctx.camera->getPosition().y,
                          ctx.camera->getPosition().z);
    }

    // ---- 从 ctx 读取 G-Buffer 纹理 ----
    m_shader->setFloat("shininess", ctx.shininess);

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
    glBindTexture(GL_TEXTURE_2D, ctx.shadowTex);
    m_shader->setInt("u_shadowTexture", 4);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    m_shader->unuse();
    m_frameBuffer.Unbind();

    // ---- 将光照纹理写入上下文 ----
    ctx.lightingTex = m_lightingTexture;
}

void LightingPass::Resize(int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, m_lightingTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

RENDERER_NAMESPACE_END
