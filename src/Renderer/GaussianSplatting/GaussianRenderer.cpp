#include "GaussianSplatting/GaussianRenderer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <glad/glad.h>
#include "Logger/Log.h"

RENDERER_NAMESPACE_BEGIN

GaussianRenderer::GaussianRenderer()
    : m_shader(Renderer::Shader::fromFiles("res/shaders/point.vs.glsl", "res/shaders/point.fs.glsl")),
      m_splatShader(Renderer::Shader::fromFiles("res/shaders/gaussian_splat.vs.glsl", "res/shaders/gaussian_splat.fs.glsl")) {
    LOG_INFO("GaussianRenderer创建成功");
}

GaussianRenderer::~GaussianRenderer() {
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_splatVAO != 0) glDeleteVertexArrays(1, &m_splatVAO);
    if (m_splatVBO != 0) glDeleteBuffers(1, &m_splatVBO);
    if (m_quadVBO != 0) glDeleteBuffers(1, &m_quadVBO);
    if (m_instanceVBO != 0) glDeleteBuffers(1, &m_instanceVBO);
}

void GaussianRenderer::loadModel(const std::string& path) 
{
    std::ifstream ifs(path, std::ios_base::binary);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open model file: " << path << std::endl;
        return;
    }

    // std::string line;
    // while (std::getline(ifs, line)) {
    //     std::istringstream iss(line);

    std::string buffer;
    std::getline(ifs, buffer);
    std::getline(ifs, buffer);
    std::getline(ifs, buffer);

    std::stringstream ss(buffer);
    std::string dummy;
    ss >> dummy;
    ss >> dummy;
    int count;
    ss >> count;
    std::cout << "Load model: " << path << " with " << count << " points" << std::endl;

    while (buffer != "end_header") {
        std::getline(ifs, buffer);
    }

    m_vertexCount = count;
    m_gaussianPoints.resize(m_vertexCount);
    ifs.read(reinterpret_cast<char*>(m_gaussianPoints.data()), m_vertexCount * sizeof(GaussianPoint<3>));

    // 计算模型的边界框
    float minX = 1e10f, minY = 1e10f, minZ = 1e10f;
    float maxX = -1e10f, maxY = -1e10f, maxZ = -1e10f;
    
    for (size_t i = 0; i < m_vertexCount; i++) 
    {
        m_gaussianPoints[i].position[1] = -m_gaussianPoints[i].position[1];
        
        // 更新边界框
        minX = std::min(minX, m_gaussianPoints[i].position[0]);
        minY = std::min(minY, m_gaussianPoints[i].position[1]);
        minZ = std::min(minZ, m_gaussianPoints[i].position[2]);
        maxX = std::max(maxX, m_gaussianPoints[i].position[0]);
        maxY = std::max(maxY, m_gaussianPoints[i].position[1]);
        maxZ = std::max(maxZ, m_gaussianPoints[i].position[2]);
    }
    ifs.close();

    setupBuffers();  // 点云渲染需要这个
    setupSplatBuffers();
    
    float centerX = (minX + maxX) * 0.5f;
    float centerY = (minY + maxY) * 0.5f;
    float centerZ = (minZ + maxZ) * 0.5f;
    float size = std::max({maxX - minX, maxY - minY, maxZ - minZ});
    
    LOG_INFO("=== 高斯模型信息 ===");
    LOG_INFO("点数量: {}", m_vertexCount);
    LOG_INFO("边界: X[{:.2f}, {:.2f}], Y[{:.2f}, {:.2f}], Z[{:.2f}, {:.2f}]", 
             minX, maxX, minY, maxY, minZ, maxZ);
    LOG_INFO("中心: ({:.2f}, {:.2f}, {:.2f})", centerX, centerY, centerZ);
    LOG_INFO("最大尺寸: {:.2f}", size);
    LOG_INFO("建议相机位置: ({:.2f}, {:.2f}, {:.2f})", 
             centerX, centerY, centerZ + size * 1.5f);
    LOG_INFO("==================");
}

void GaussianRenderer::drawPoints(const Renderer::Matrix4& model, const Renderer::Matrix4& view, const Renderer::Matrix4& projection)
{
    m_shader.use();
    m_shader.setMat4("model", model.data());
    m_shader.setMat4("view", view.data());
    m_shader.setMat4("projection", projection.data());
    glPointSize(3.0f);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_POINTS, 0, m_vertexCount);
    glBindVertexArray(0);
}

void GaussianRenderer::setupBuffers()
{
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    std::cout << "Size of GaussianPoint<3>: " << sizeof(GaussianPoint<3>) << std::endl;
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertexCount * sizeof(GaussianPoint<3>), m_gaussianPoints.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GaussianPoint<3>), (void*)offsetof(GaussianPoint<3>, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GaussianPoint<3>), (void*)offsetof(GaussianPoint<3>, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(GaussianPoint<3>), (void*)offsetof(GaussianPoint<3>, shs));

    glBindVertexArray(0);
}

void GaussianRenderer::setupSplatBuffers()
{
    // 清理旧的缓冲区
    if (m_splatVAO != 0) glDeleteVertexArrays(1, &m_splatVAO);
    if (m_splatVBO != 0) glDeleteBuffers(1, &m_splatVBO);
    if (m_quadVBO != 0) glDeleteBuffers(1, &m_quadVBO);
    if (m_instanceVBO != 0) glDeleteBuffers(1, &m_instanceVBO);

    // 创建VAO
    glGenVertexArrays(1, &m_splatVAO);
    glBindVertexArray(m_splatVAO);

    // 1. 创建高斯数据VBO（每个实例的数据）
    glGenBuffers(1, &m_splatVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_splatVBO);
    glBufferData(GL_ARRAY_BUFFER, m_vertexCount * sizeof(GaussianPoint<3>), m_gaussianPoints.data(), GL_STATIC_DRAW);

    // 设置顶点属性（per instance）
    size_t stride = sizeof(GaussianPoint<3>);
    
    // 位置 (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GaussianPoint<3>, position));
    glVertexAttribDivisor(0, 1);  // 每个实例更新一次
    
    // 法线 (location 1) - 暂不使用
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GaussianPoint<3>, normal));
    glVertexAttribDivisor(1, 1);
    
    // SH颜色 (location 2) - 只使用第一个3分量
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GaussianPoint<3>, shs));
    glVertexAttribDivisor(2, 1);
    
    // 不透明度 (location 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GaussianPoint<3>, opacity));
    glVertexAttribDivisor(3, 1);
    
    // 缩放 (location 4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GaussianPoint<3>, scale));
    glVertexAttribDivisor(4, 1);
    
    // 旋转四元数 (location 5)
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GaussianPoint<3>, rotation));
    glVertexAttribDivisor(5, 1);

    // 2. 创建四边形顶点VBO（每个顶点的偏移）
    float quadVertices[] = {
        // 两个三角形组成一个四边形
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    
    glGenBuffers(1, &m_quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // 四边形偏移 (location 6) - 每个顶点都不同
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glVertexAttribDivisor(6, 0);  // 每个顶点更新一次（不是每个实例）

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GaussianRenderer::sortGaussiansByDepth(const Renderer::Matrix4& view)
{
    // 创建或更新排序索引
    if (m_sortedIndices.size() != m_vertexCount) {
        m_sortedIndices.resize(m_vertexCount);
        for (uint32_t i = 0; i < m_vertexCount; i++) {
            m_sortedIndices[i] = i;
        }
    }

    // 计算每个高斯在视图空间的深度，并排序
    std::vector<std::pair<float, uint32_t>> depthIndices;
    depthIndices.reserve(m_vertexCount);

    const float* viewData = view.data();
    
    for (uint32_t i = 0; i < m_vertexCount; i++) {
        const float* pos = m_gaussianPoints[i].position;
        
        // 手动计算 view * position（只需要z分量）
        float viewZ = viewData[8] * pos[0] + viewData[9] * pos[1] + 
                      viewData[10] * pos[2] + viewData[11];
        
        depthIndices.push_back({viewZ, i});
    }

    // 从后向前排序（深度从大到小）
    std::sort(depthIndices.begin(), depthIndices.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });

    // 更新排序后的索引
    for (size_t i = 0; i < depthIndices.size(); i++) {
        m_sortedIndices[i] = depthIndices[i].second;
    }
}

void GaussianRenderer::drawSplats(const Renderer::Matrix4& model, const Renderer::Matrix4& view, 
                                   const Renderer::Matrix4& projection, int width, int height)
{
    if (m_vertexCount == 0 || m_splatVAO == 0) {
        LOG_WARN("drawSplats: 没有数据或VAO未初始化");
        return;
    }

    try {
        // 1. 深度排序（对于透明度混合很重要）
        sortGaussiansByDepth(view);
        
        // 根据排序索引重新排列高斯数据
        static std::vector<GaussianPoint<3>> sortedGaussians;
        if (sortedGaussians.size() != m_vertexCount) {
            sortedGaussians.resize(m_vertexCount);
        }
        for (uint32_t i = 0; i < m_vertexCount; i++) {
            sortedGaussians[i] = m_gaussianPoints[m_sortedIndices[i]];
        }
        
        // 更新VBO数据（使用排序后的数据）
        glBindBuffer(GL_ARRAY_BUFFER, m_splatVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexCount * sizeof(GaussianPoint<3>), sortedGaussians.data());
        
        // 记录第一帧
        static bool firstFrame = true;
        if (firstFrame) {
            LOG_INFO("开始渲染 {} 个高斯点（已启用深度排序）", m_vertexCount);
            firstFrame = false;
        }

        // 2. 启用混合（用于透明度和splat叠加）
        glEnable(GL_BLEND);
        // 使用预乘alpha的混合模式
        glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);

        // 3. 禁用深度写入但启用深度测试
        glDepthMask(GL_FALSE);
        glEnable(GL_DEPTH_TEST);  // 启用深度测试获得正确的遮挡

        // 4. 使用splat着色器
        m_splatShader.use();
        m_splatShader.setMat4("uModel", model.data());
        m_splatShader.setMat4("uView", view.data());
        m_splatShader.setMat4("uProjection", projection.data());
        m_splatShader.setVec2("uViewport", (float)width, (float)height);
        
        // 简化的焦距计算（假设FOV=45度）
        float fov = 45.0f * 3.14159265f / 180.0f;
        float focal = height / (2.0f * std::tan(fov / 2.0f));
        m_splatShader.setFloat("uFocalX", focal);
        m_splatShader.setFloat("uFocalY", focal);
        
        // 相机位置（从view矩阵的逆中提取）
        Matrix4 invView = view.inverse();
        m_splatShader.setVec3("uCameraPos", invView.m[12], invView.m[13], invView.m[14]);

        // 5. 绑定VAO并绘制（使用实例化渲染）
        glBindVertexArray(m_splatVAO);
        
        // 检查OpenGL错误
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("OpenGL错误（绑定VAO前）: {}", err);
        }
        
        // 按排序顺序绘制高斯（作为实例）
        // 性能优化：可以只渲染部分点来提升帧率
        // uint32_t renderCount = m_vertexCount / 2;  // 渲染一半的点
        // uint32_t renderCount = m_vertexCount / 4;  // 渲染1/4的点
        uint32_t renderCount = m_vertexCount;  // 渲染所有点
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, renderCount);
        
        err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("OpenGL错误（绘制后）: {}", err);
        }

        glBindVertexArray(0);

        // 6. 恢复状态
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    } catch (const std::exception& e) {
        LOG_ERROR("drawSplats异常: {}", e.what());
    }
}

RENDERER_NAMESPACE_END