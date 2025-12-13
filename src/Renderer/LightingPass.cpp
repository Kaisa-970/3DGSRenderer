#include "LightingPass.h"
#include <glad/glad.h>
#include "RenderHelper/RenderHelper.h"

RENDERER_NAMESPACE_BEGIN

static const float vertices[] = {
    -1.0f, -1.0f, 0.0f, 0.0f,
    1.0f, -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 0.0f, 1.0f
};

LightingPass::LightingPass(const int& width, const int& height)
    : m_shader(Renderer::Shader::fromFiles("res/shaders/lambert.vs.glsl", "res/shaders/lambert.fs.glsl")) {

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_lightingTexture = RenderHelper::CreateTexture2D(width, height, GL_RGB32F, GL_RGB, GL_FLOAT);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Color0, m_lightingTexture);
}

LightingPass::~LightingPass() {
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color0);
    if (m_lightingTexture != 0) glDeleteTextures(1, &m_lightingTexture);
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
}

void LightingPass::Begin(const Camera& camera, const Renderer::Vector3& lightPos) {
    m_frameBuffer.Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    m_shader->use();
    m_shader->setVec3("lightPos", lightPos.x, lightPos.y, lightPos.z);       // 光源位置
    m_shader->setVec3("lightColor", 1.0f, 1.0f, 1.0f);     // 白光
    m_shader->setVec3("viewPos", camera.getPosition().x, camera.getPosition().y, camera.getPosition().z);     // 相机位置

    glDisable(GL_DEPTH_TEST);
}

void LightingPass::Render(const unsigned int& positionTexture, const unsigned int& normalTexture, const unsigned int& diffuseTexture, const unsigned int& specularTexture, const unsigned int& shininessTexture) {
    // 设置光照强度（可选，有默认值）
    m_shader->setFloat("ambientStrength", 0.1f);    // 环境光强度
    m_shader->setFloat("diffuseStrength", 0.9f);    // 漫反射强度
    m_shader->setFloat("specularStrength", 0.5f);   // 镜面反射强度
    m_shader->setInt("shininess", 32);    

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, positionTexture);
    m_shader->setInt("u_positionTexture", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    m_shader->setInt("u_normalTexture", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, diffuseTexture);
    m_shader->setInt("u_diffuseTexture", 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, specularTexture);
    m_shader->setInt("u_specularTexture", 3);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, shininessTexture);
    m_shader->setInt("u_shininessTexture", 4);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

void LightingPass::End() {
    m_frameBuffer.Unbind();
    m_shader->unuse();
}

RENDERER_NAMESPACE_END