#pragma once

#include "Core/RenderCore.h"
#include "Shader.h"
#include "FrameBuffer.h"
#include "Camera.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API PostProcessPass {
public:
    PostProcessPass(const int& width, const int& height);
    ~PostProcessPass();

    void render(int width, int height, Camera& camera, const unsigned int currentSelectedUID, 
        const unsigned int& uidTexture, const unsigned int& positionTexture, const unsigned int& normalTexture, const unsigned int& lightingTexture, const unsigned int& depthTexture, const unsigned int& gaussianTexture);

    unsigned int getColorTexture() const { return m_colorTexture; }

private:
    Shader m_shader;
    FrameBuffer m_frameBuffer;
    unsigned int m_colorTexture;
    unsigned int m_vao;
    unsigned int m_vbo;
};

RENDERER_NAMESPACE_END