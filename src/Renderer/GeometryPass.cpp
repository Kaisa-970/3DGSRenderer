#include "GeometryPass.h"
#include <glad/glad.h>
#include "RenderHelper/RenderHelper.h"
#include "MaterialManager.h"

RENDERER_NAMESPACE_BEGIN

GeometryPass::GeometryPass(const int& width, const int& height)
    : m_shader(Renderer::Shader::fromFiles("res/shaders/basepass.vs.glsl", "res/shaders/basepass.fs.glsl")) {
        m_positionTexture = RenderHelper::CreateTexture2D(width, height, GL_RGB32F, GL_RGB, GL_FLOAT);
        m_normalTexture = RenderHelper::CreateTexture2D(width, height, GL_RGB32F, GL_RGB, GL_FLOAT);
        m_diffuseTexture = RenderHelper::CreateTexture2D(width, height, GL_RGB32F, GL_RGB, GL_FLOAT);
        m_specularTexture = RenderHelper::CreateTexture2D(width, height, GL_RGB32F, GL_RGB, GL_FLOAT);
        m_shininessTexture = RenderHelper::CreateTexture2D(width, height, GL_R32F, GL_RED, GL_FLOAT);
        m_uidTexture = RenderHelper::CreateTexture2D(width, height, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
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
            GL_COLOR_ATTACHMENT0, 
            GL_COLOR_ATTACHMENT1, 
            GL_COLOR_ATTACHMENT2,
            GL_COLOR_ATTACHMENT3,
            GL_COLOR_ATTACHMENT4,
            GL_COLOR_ATTACHMENT5,
            //GL_COLOR_ATTACHMENT3  // 对应 gDepth（虽然是float，但仍作为颜色附件）
        };
        glDrawBuffers(6, drawBuffers);
        m_frameBuffer.Unbind();
    }

GeometryPass::~GeometryPass() {
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color0);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color1);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color2);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color3);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color4);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color5);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Depth);
    if (m_positionTexture != 0) glDeleteTextures(1, &m_positionTexture);
    if (m_normalTexture != 0) glDeleteTextures(1, &m_normalTexture);
    if (m_diffuseTexture != 0) glDeleteTextures(1, &m_diffuseTexture);
    if (m_specularTexture != 0) glDeleteTextures(1, &m_specularTexture);
    if (m_shininessTexture != 0) glDeleteTextures(1, &m_shininessTexture);
    if (m_uidTexture != 0) glDeleteTextures(1, &m_uidTexture);
    if (m_depthTexture != 0) glDeleteTextures(1, &m_depthTexture);
}

void GeometryPass::Begin(const float* view, const float* projection) {
    m_frameBuffer.Bind();
    m_frameBuffer.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    m_frameBuffer.ClearDepthStencil(1.0f, 0);
    m_shader.use();
    m_shader.setMat4("view", view);
    m_shader.setMat4("projection", projection);

    m_frameBuffer.Bind();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

void GeometryPass::Render(Primitive* primitive, unsigned int uid, const float* model, const Vector3& color) {
    m_shader.setMat4("model", model);
    m_shader.setVec3("uColor", color.x, color.y, color.z);
    m_shader.setInt("uUID", static_cast<int>(uid));

    std::shared_ptr<Material> material = MaterialManager::GetInstance()->GetDefaultMaterial();
    if (material) {
        m_shader.setVec3("u_diffuseColor", material->getDiffuseColor().x, material->getDiffuseColor().y, material->getDiffuseColor().z);
        m_shader.setVec3("u_specularColor", material->getSpecularColor().x, material->getSpecularColor().y, material->getSpecularColor().z);
        m_shader.setFloat("u_shininess", material->getShininess());
        if (material->getDiffuseTextures().size() > 0) {
            material->getDiffuseTextures()[0]->bind(0);
            m_shader.setInt("u_diffuseTexture", 0);
        }
        if (material->getSpecularTextures().size() > 0) {
            material->getSpecularTextures()[0]->bind(1);
            m_shader.setInt("u_specularTexture", 1);
        }
        if (material->getNormalTextures().size() > 0) {
            material->getNormalTextures()[0]->bind(2);
            m_shader.setInt("u_normalTexture", 2);
        }
    }

    primitive->draw(m_shader);
}  

void GeometryPass::Render(Model* model, unsigned int uid, const float* modelMatrix) {
    m_shader.setMat4("model", modelMatrix);
    m_shader.setVec3("uColor", 1.0f, 1.0f, 1.0f);
    m_shader.setInt("uUID", static_cast<int>(uid));
    model->draw(m_shader);
}

void GeometryPass::Render(Renderable* renderable) {
    renderable->draw(m_shader);
}

void GeometryPass::End() {
    m_frameBuffer.Unbind();
    m_shader.unuse();
}

unsigned int GeometryPass::getCurrentSelectedUID(unsigned int mouseX, unsigned int mouseY) {
    m_frameBuffer.Bind();
    glReadBuffer(GL_COLOR_ATTACHMENT5);
    unsigned int uid;
    glReadPixels(mouseX, mouseY, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &uid);
    m_frameBuffer.Unbind();
    return uid;
}

RENDERER_NAMESPACE_END