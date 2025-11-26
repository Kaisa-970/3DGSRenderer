#pragma once

#include "Core/RenderCore.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API FrameBuffer {
public:
    FrameBuffer();
    ~FrameBuffer();

    enum class Attachment{
        Color0 = 0,
        Color1,
        Color2,
        Color3,
        Color4,
        Color5,
        Color6,
        Color7,
        Depth
    };

    void Attach(Attachment attachment, unsigned int textureId);
    void Detach(Attachment attachment);
    void Bind() const;
    void Unbind() const;
    void Resize(int width, int height);
    // void Clear(float r, float g, float b, float a);
    // void ClearDepth(float depth);
    // void ClearStencil(int stencil);
    void ClearColor(float r, float g, float b, float a);
    void ClearDepthStencil(float depth, int stencil);

private:
    unsigned int m_bufferId;
};

RENDERER_NAMESPACE_END