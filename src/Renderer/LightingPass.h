#pragma once

#include "Core/RenderCore.h"
#include "Shader.h"
#include "Camera.h"
#include "Light.h"
#include "FrameBuffer.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API LightingPass {
public:
    LightingPass(const int& width, const int& height);
    ~LightingPass();

    void Begin(const Camera& camera, const Light& light);
    void Render(const unsigned int& positionTexture, const unsigned int& normalTexture, const unsigned int& diffuseTexture, const unsigned int& specularTexture, const unsigned int& shininessTexture);
    void End();

    unsigned int getLightingTexture() const { return m_lightingTexture; }

private:
    std::shared_ptr<Shader> m_shader;
    FrameBuffer m_frameBuffer;
    unsigned int m_lightingTexture;
    unsigned int m_vao;
    unsigned int m_vbo;
};

RENDERER_NAMESPACE_END