#include "FrameBuffer.h"
#include <glad/glad.h>
#include "Logger/Log.h"

RENDERER_NAMESPACE_BEGIN

FrameBuffer::FrameBuffer() {
    glGenFramebuffers(1, &m_bufferId);
}

FrameBuffer::~FrameBuffer() {
    glDeleteFramebuffers(1, &m_bufferId);
}

void FrameBuffer::Bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_bufferId);
}

void FrameBuffer::Unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::Attach(Attachment attachment, unsigned int textureId) {
    Bind();
    int index = static_cast<int>(attachment);
    int depthIdx = static_cast<int>(Attachment::Depth);
    if (index < depthIdx) {
        GLenum attachmentType = GL_COLOR_ATTACHMENT0 + index;
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, textureId, 0);
    } else {
        GLenum attachmentType = GL_DEPTH_ATTACHMENT;
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, textureId, 0);
    }
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_CORE_ERROR("Failed to attach texture to framebuffer: {}", status);
        Unbind();
        return;
    }
    Unbind();
}

void FrameBuffer::Detach(Attachment attachment) {
    Bind();
    int index = static_cast<int>(attachment);
    int depthIdx = static_cast<int>(Attachment::Depth);
    if (index < depthIdx) {
        GLenum attachmentType = GL_COLOR_ATTACHMENT0 + index;
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, 0, 0);
    } else {
        GLenum attachmentType = GL_DEPTH_ATTACHMENT;
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, 0, 0);
    }
    Unbind();
}

void FrameBuffer::Resize(int width, int height) {
    Bind();
    glViewport(0, 0, width, height);
    Unbind();
}

void FrameBuffer::ClearColor(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void FrameBuffer::ClearDepthStencil(float depth, int stencil) {
    glClearDepthf(depth);
    glClearStencil(stencil);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

RENDERER_NAMESPACE_END