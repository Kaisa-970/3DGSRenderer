#pragma once

#include "Core/RenderCore.h"
#include "Shader.h"
#include "Camera.h"
#include "FrameBuffer.h"
#include "GaussianSplatting/GaussianRenderer.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API LightingPass {
public:
    LightingPass();
    ~LightingPass();

    void render(int width, int height, Camera& camera, const Renderer::Vector3& lightPos, const unsigned int& positionTexture, const unsigned int& normalTexture, const unsigned int& colorTexture);

    unsigned int getLightingTexture() const { return m_lightingTexture; }

private:
    Shader m_shader;
    FrameBuffer m_frameBuffer;
    unsigned int m_lightingTexture;
    unsigned int m_vao;
    unsigned int m_vbo;
};

RENDERER_NAMESPACE_END