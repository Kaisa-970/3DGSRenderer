#pragma once

#include "Core/RenderCore.h"
#include "FrameBuffer.h"
#include "Shader.h"
#include "Primitives/Primitive.h"
#include <vector>
#include "MathUtils/Matrix.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API GeometryPass {
public:
    GeometryPass();
    ~GeometryPass();

    void clear();
    void render(Primitive* primitive, const float* model, const float* view, const float* projection, const Vector3& color = Vector3(1.0f, 1.0f, 1.0f));

    unsigned int getPositionTexture() const { return m_positionTexture; }
    unsigned int getNormalTexture() const { return m_normalTexture; }
    unsigned int getColorTexture() const { return m_colorTexture; }
    unsigned int getDepthTexture() const { return m_depthTexture; }
private:
    FrameBuffer m_frameBuffer;
    Shader m_shader;
    unsigned int m_positionTexture;
    unsigned int m_normalTexture;
    unsigned int m_colorTexture;
    unsigned int m_depthTexture;

    //std::vector<Primitive*> m_primitives;
};

RENDERER_NAMESPACE_END