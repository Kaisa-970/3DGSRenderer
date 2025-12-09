#pragma once

#include "Core/RenderCore.h"
#include "FrameBuffer.h"
#include "Shader.h"
#include "Primitives/Primitive.h"
#include <vector>
#include "MathUtils/Matrix.h"
#include "Model.h"
RENDERER_NAMESPACE_BEGIN

class RENDERER_API GeometryPass {
public:
    GeometryPass(const int& width, const int& height);
    ~GeometryPass();

    void Begin(const float* view, const float* projection);
    void Render(Primitive* primitive, const float* model, const Vector3& color = Vector3(1.0f, 1.0f, 1.0f));
    void Render(Model* model, const float* modelMatrix);
    void End();
    //void render(Model* model, const float* model, const float* view, const float* projection, const Vector3& color = Vector3(1.0f, 1.0f, 1.0f));
    unsigned int getPositionTexture() const { return m_positionTexture; }
    unsigned int getNormalTexture() const { return m_normalTexture; }
    unsigned int getDiffuseTexture() const { return m_diffuseTexture; }
    unsigned int getSpecularTexture() const { return m_specularTexture; }
    unsigned int getShininessTexture() const { return m_shininessTexture; }
    unsigned int getDepthTexture() const { return m_depthTexture; }
private:
    FrameBuffer m_frameBuffer;
    Shader m_shader;
    unsigned int m_positionTexture;
    unsigned int m_normalTexture;
    unsigned int m_diffuseTexture;
    unsigned int m_specularTexture;
    unsigned int m_shininessTexture;
    unsigned int m_depthTexture;

    //std::vector<Primitive*> m_primitives;
};

RENDERER_NAMESPACE_END