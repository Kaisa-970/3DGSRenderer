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

void GeometryPass::begin(const float* view, const float* projection) {
    memcpy(m_view, view, sizeof(float) * 16);
    memcpy(m_projection, projection, sizeof(float) * 16);
    m_frameBuffer.Bind();
    m_frameBuffer.ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    m_frameBuffer.ClearDepthStencil(1.0f, 0);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    m_shader.use();
    m_shader.setMat4("view", m_view);
    m_shader.setMat4("projection", m_projection);
}

void GeometryPass::render(Primitive* primitive, const float* model, const Vector3& color) {

    m_shader.setMat4("model", model);
    m_shader.setVec3("uColor", color.x, color.y, color.z);

    primitive->draw(m_shader);
}   

void GeometryPass::end() {
    m_frameBuffer.Unbind();
    m_shader.unuse();
}

RENDERER_NAMESPACE_END