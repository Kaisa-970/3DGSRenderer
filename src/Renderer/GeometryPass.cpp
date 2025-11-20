#include "GeometryPass.h"
#include <glad/glad.h>
#include "RenderHelper/RenderHelper.h"

RENDERER_NAMESPACE_BEGIN

GeometryPass::GeometryPass()
    : m_shader(Renderer::Shader::fromFiles("res/shaders/basepass.vs.glsl", "res/shaders/basepass.fs.glsl")) {
        m_positionTexture = RenderHelper::CreateTexture2D(1600, 900, GL_RGB32F, GL_RGB, GL_FLOAT);
        m_normalTexture = RenderHelper::CreateTexture2D(1600, 900, GL_RGB32F, GL_RGB, GL_FLOAT);
        m_colorTexture = RenderHelper::CreateTexture2D(1600, 900, GL_RGB32F, GL_RGB, GL_FLOAT);
        m_depthTexture = RenderHelper::CreateTexture2D(1600, 900, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT);
        
        m_frameBuffer.Attach(FrameBuffer::Attachment::Color0, m_positionTexture);
        m_frameBuffer.Attach(FrameBuffer::Attachment::Color1, m_normalTexture);
        m_frameBuffer.Attach(FrameBuffer::Attachment::Color2, m_colorTexture);
        m_frameBuffer.Attach(FrameBuffer::Attachment::Depth, m_depthTexture);
    
        m_frameBuffer.Bind();
        GLenum drawBuffers[] = { 
            GL_COLOR_ATTACHMENT0, 
            GL_COLOR_ATTACHMENT1, 
            GL_COLOR_ATTACHMENT2,
            //GL_COLOR_ATTACHMENT3  // 对应 gDepth（虽然是float，但仍作为颜色附件）
        };
        glDrawBuffers(3, drawBuffers);
        m_frameBuffer.Unbind();
    }

GeometryPass::~GeometryPass() {
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color0);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color1);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color2);
    m_frameBuffer.Detach(FrameBuffer::Attachment::Depth);
    if (m_positionTexture != 0) glDeleteTextures(1, &m_positionTexture);
    if (m_normalTexture != 0) glDeleteTextures(1, &m_normalTexture);
    if (m_colorTexture != 0) glDeleteTextures(1, &m_colorTexture);
    if (m_depthTexture != 0) glDeleteTextures(1, &m_depthTexture);
}

void GeometryPass::clear() {
    m_frameBuffer.Bind();
    m_frameBuffer.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    m_frameBuffer.ClearDepthStencil(1.0f, 0);
    m_frameBuffer.Unbind();
}

void GeometryPass::render(Primitive* primitive, const float* model, const float* view, const float* projection, const Vector3& color) {
    m_frameBuffer.Bind();
    // m_frameBuffer.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    // glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    m_shader.use();
    m_shader.setMat4("model", model);
    m_shader.setMat4("view", view);
    m_shader.setMat4("projection", projection);
    m_shader.setVec3("uColor", color.x, color.y, color.z);

    primitive->draw(m_shader);

    m_frameBuffer.Unbind(); 
}   

RENDERER_NAMESPACE_END