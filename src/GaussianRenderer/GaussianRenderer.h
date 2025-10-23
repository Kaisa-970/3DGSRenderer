#pragma once

#include "TypeDef.h"
#include <string>
#include <vector>
#include "Renderer/Shader.h"
#include "Renderer/MathUtils/Matrix.h"

GAUSSIAN_RENDERER_NAMESPACE_BEGIN

class GAUSSIAN_RENDERER_API GaussianRenderer {
public:
    GaussianRenderer();
    ~GaussianRenderer();

    void loadModel(const std::string& path);
    void drawPoints(const Renderer::Matrix4& model, const Renderer::Matrix4& view, const Renderer::Matrix4& projection);

private:
    void setupBuffers();

private:
    std::vector<NormalPoint> m_points;
    Renderer::Shader m_shader;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_vertexCount = 0;
};

GAUSSIAN_RENDERER_NAMESPACE_END