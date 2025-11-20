#pragma once

#include "Core/TypeDef.h"
#include <string>
#include <vector>
#include "Renderer/Shader.h"
#include "Renderer/MathUtils/Matrix.h"
#include "Renderer/FrameBuffer.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API GaussianRenderer {
public:
    GaussianRenderer();
    ~GaussianRenderer();

    void loadModel(const std::string& path);
    void drawPoints(const Renderer::Matrix4& model, const Renderer::Matrix4& view, const Renderer::Matrix4& projection);
    
    // 高质量splat渲染
    void drawSplats(const Renderer::Matrix4& model, const Renderer::Matrix4& view, const Renderer::Matrix4& projection, int width, int height);

    unsigned int getColorTexture() const { return m_colorTexture; }

private:
    void setupBuffers();
    void setupSplatBuffers();
    void sortGaussiansByDepth(const Renderer::Matrix4& view);

private:
    std::vector<NormalPoint> m_points;
    std::vector<GaussianPoint<3>> m_gaussianPoints;
    std::vector<uint32_t> m_sortedIndices;  // 排序后的索引
    
    Renderer::Shader m_shader;
    Renderer::Shader m_splatShader;
    
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_vertexCount = 0;
    
    // Splat渲染的VAO和VBO
    unsigned int m_splatVAO = 0;
    unsigned int m_splatVBO = 0;  
    unsigned int m_quadVAO = 0;        // 高斯数据
    unsigned int m_quadVBO = 0;           // 四边形顶点数据
    unsigned int m_instanceVBO = 0;       // 实例数据（排序后的索引）
    unsigned int m_orderSSBO = 0;  
    
    FrameBuffer* m_frameBuffer = nullptr; // 帧缓冲区
    unsigned int m_colorTexture = 0; // 颜色纹理
};

RENDERER_NAMESPACE_END