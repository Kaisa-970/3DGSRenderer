#include "GaussianSplatting/GaussianRenderer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <glad/glad.h>
#include "Logger/Log.h"
#include "MathUtils/Vector.h"

RENDERER_NAMESPACE_BEGIN

float sigmoid(float x)
{
    return 1.0f / (1.0f + std::exp(-x));
}

unsigned int setupSSBO(const unsigned int& bindIdx, const float* bufferData, const size_t& bufferSize)
{
    unsigned int ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize * sizeof(float), bufferData, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindIdx, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return ssbo;
}

unsigned int setupSSBO_int(const unsigned int& bindIdx, const int* bufferData, const size_t& bufferSize)
{
    unsigned int ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize * sizeof(int), bufferData, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindIdx, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return ssbo;
}

GaussianRenderer::GaussianRenderer()
    : m_shader(Renderer::Shader::fromFiles("res/shaders/point.vs.glsl", "res/shaders/point.fs.glsl")),
      m_splatShader(Renderer::Shader::fromFiles("res/shaders/gaussian.vs.glsl", "res/shaders/gaussian.fs.glsl")) {
    LOG_INFO("GaussianRenderer创建成功");
}

GaussianRenderer::~GaussianRenderer() {
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_splatVAO != 0) glDeleteVertexArrays(1, &m_splatVAO);
    if (m_splatVBO != 0) glDeleteBuffers(1, &m_splatVBO);
    if (m_quadVBO != 0) glDeleteBuffers(1, &m_quadVBO);
    if (m_instanceVBO != 0) glDeleteBuffers(1, &m_instanceVBO);
    if (m_orderSSBO != 0) glDeleteBuffers(1, &m_orderSSBO);
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

        // 旋转归一化
        float angle = std::sqrt(m_gaussianPoints[i].rotation[0] * m_gaussianPoints[i].rotation[0] + m_gaussianPoints[i].rotation[1] * m_gaussianPoints[i].rotation[1] + m_gaussianPoints[i].rotation[2] * m_gaussianPoints[i].rotation[2]);
        m_gaussianPoints[i].rotation[0] = m_gaussianPoints[i].rotation[0] / angle;
        m_gaussianPoints[i].rotation[1] = m_gaussianPoints[i].rotation[1] / angle;
        m_gaussianPoints[i].rotation[2] = m_gaussianPoints[i].rotation[2] / angle;
        m_gaussianPoints[i].rotation[3] = m_gaussianPoints[i].rotation[3] / angle;

        m_gaussianPoints[i].scale[0] = std::exp(m_gaussianPoints[i].scale[0]);
        m_gaussianPoints[i].scale[1] = std::exp(m_gaussianPoints[i].scale[1]);
        m_gaussianPoints[i].scale[2] = std::exp(m_gaussianPoints[i].scale[2]);

        m_gaussianPoints[i].opacity = sigmoid(m_gaussianPoints[i].opacity);
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
    if (m_quadVAO != 0) glDeleteVertexArrays(1, &m_quadVAO);

    float quadVertices[] = {
        -1.0f, -1.0f,
            1.0f, -1.0f,
        -1.0f,  1.0f,
            1.0f, -1.0f,
            1.0f,  1.0f,
        -1.0f,  1.0f
    };
    
    glGenVertexArrays(1, &m_quadVAO);
    glBindVertexArray(m_quadVAO);

    glGenBuffers(1, &m_quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint pointsBindIdx = 1;
    GLuint pointsSSBO = setupSSBO(pointsBindIdx, reinterpret_cast<const float*>(m_gaussianPoints.data()), m_vertexCount * sizeof(GaussianPoint<3>) / sizeof(float));
    
    GLuint orderBindIdx = 2;
    m_sortedIndices.resize(m_vertexCount, 0);
    for (uint32_t i = 0; i < m_vertexCount; i++) {
        m_sortedIndices[i] = i;
    }
    m_orderSSBO = setupSSBO_int(orderBindIdx, reinterpret_cast<const int*>(m_sortedIndices.data()), m_vertexCount * sizeof(uint32_t) / sizeof(int));
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
    // if (m_vertexCount == 0 || m_splatVAO == 0) {
    //     LOG_WARN("drawSplats: 没有数据或VAO未初始化");
    //     return;
    // }

    try {
        LOG_INFO("开始drawSplats: 顶点数={}", m_vertexCount);
        
        // 1. 深度排序
        sortGaussiansByDepth(view);
        LOG_INFO("排序完成");
        
        // 2. 更新GPU上的排序索引
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_orderSSBO);
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("绑定SSBO错误: {}", err);
            return;
        }
        
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 
            m_vertexCount * sizeof(int), 
            reinterpret_cast<const int*>(m_sortedIndices.data()));
        
        err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("更新SSBO数据错误: {}", err);
            return;
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        LOG_INFO("SSBO更新完成");
        
        // 3. 启用混合
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glDepthMask(GL_FALSE);  // 禁用深度写入（但保持深度测试）
        //glEnable(GL_DEPTH_TEST); // 启用深度测试
        
        // 4. 计算焦距参数
        float fov = 45.0f * 3.14159265f / 180.0f;
        float htany = std::tan(fov / 2.0f);
        float htanx = std::tan(fov / 2.0f) * width / height;
        float focal_x = width / (2.0f * htanx);
        float focal_y = height / (2.0f * htany);
        
        // 5. 使用shader并设置uniform
        m_splatShader.use();
        m_splatShader.setMat4("view", view.data());
        m_splatShader.setMat4("projection", projection.data());
        m_splatShader.setVec4("hfov_focal", htanx, htany, focal_x, focal_y);
        
        LOG_INFO("Shader设置完成");

        // 6. 绑定VAO
        glBindVertexArray(m_quadVAO);
        err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("绑定VAO错误: {}", err);
            return;
        }
        
        // 7. 限制渲染数量（关键！）
        // 先用小数量测试，避免GPU超时
        uint32_t renderCount = m_vertexCount;//std::min(m_vertexCount, 10000u);  // 先只渲染1万个点测试
        
        LOG_INFO("准备渲染 {} 个实例", renderCount);
        
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, renderCount);
        
        err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("绘制错误: {}", err);
            return;
        }

        LOG_CORE_INFO("渲染完成");
        glBindVertexArray(0);
        glDisable(GL_BLEND);
    } catch (const std::exception& e) {
        LOG_ERROR("drawSplats异常: {}", e.what());
    }
}

RENDERER_NAMESPACE_END