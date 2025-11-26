#include "GaussianSplatting/GaussianRenderer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <execution>
#include <glad/glad.h>
#include "Logger/Log.h"
#include "MathUtils/Vector.h"
#include "FrameBuffer.h"

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

unsigned int setupSSBO_int(const unsigned int& bindIdx, const uint32_t* bufferData, const size_t& bufferSize)
{
    unsigned int ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize * sizeof(uint32_t), bufferData, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindIdx, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return ssbo;
}

GaussianRenderer::GaussianRenderer()
    : m_shader(Renderer::Shader::fromFiles("res/shaders/point.vs.glsl", "res/shaders/point.fs.glsl")),
      m_splatShader(Renderer::Shader::fromFiles("res/shaders/gaussian.vs.glsl", "res/shaders/gaussian.fs.glsl")) {
    
    m_frameBuffer = new FrameBuffer();
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1600, 900, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_frameBuffer->Attach(FrameBuffer::Attachment::Color0, m_colorTexture);
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
    if (m_frameBuffer != nullptr) delete m_frameBuffer;
    if (m_colorTexture != 0) glDeleteTextures(1, &m_colorTexture);
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
        m_gaussianPoints[i].position[0] = m_gaussianPoints[i].position[0];
        m_gaussianPoints[i].position[1] = m_gaussianPoints[i].position[1];
        
        // 更新边界框
        minX = std::min(minX, m_gaussianPoints[i].position[0]);
        minY = std::min(minY, m_gaussianPoints[i].position[1]);
        minZ = std::min(minZ, m_gaussianPoints[i].position[2]);
        maxX = std::max(maxX, m_gaussianPoints[i].position[0]);
        maxY = std::max(maxY, m_gaussianPoints[i].position[1]);
        maxZ = std::max(maxZ, m_gaussianPoints[i].position[2]);

        // 旋转归一化
        float angle = std::sqrt(m_gaussianPoints[i].rotation[0] * m_gaussianPoints[i].rotation[0] + m_gaussianPoints[i].rotation[1] * m_gaussianPoints[i].rotation[1] 
            + m_gaussianPoints[i].rotation[2] * m_gaussianPoints[i].rotation[2] + m_gaussianPoints[i].rotation[3] * m_gaussianPoints[i].rotation[3]);
        
        
        if (angle > 0.00001f) {
            m_gaussianPoints[i].rotation[0] = m_gaussianPoints[i].rotation[0] / angle;
            m_gaussianPoints[i].rotation[1] = m_gaussianPoints[i].rotation[1] / angle;
            m_gaussianPoints[i].rotation[2] = m_gaussianPoints[i].rotation[2] / angle;
            m_gaussianPoints[i].rotation[3] = m_gaussianPoints[i].rotation[3] / angle;
        }
        else {
            m_gaussianPoints[i].rotation[0] = 1.0f;
            m_gaussianPoints[i].rotation[1] = 0.0f;
            m_gaussianPoints[i].rotation[2] = 0.0f;
            m_gaussianPoints[i].rotation[3] = 0.0f;
        }

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
    m_frameBuffer->Bind();
    m_frameBuffer->ClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    m_shader.use();
    m_shader.setMat4("model", model.data());
    m_shader.setMat4("view", view.data());
    m_shader.setMat4("projection", projection.data());
    glPointSize(3.0f);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_POINTS, 0, m_vertexCount);
    glBindVertexArray(0);
    m_frameBuffer->Unbind();
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
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint pointsBindIdx = 1;
    GLuint pointsSSBO = setupSSBO(pointsBindIdx, reinterpret_cast<const float*>(m_gaussianPoints.data()), m_vertexCount * sizeof(GaussianPoint<3>) / sizeof(float));

    GLuint orderBindIdx = 2;
    m_sortedIndices.resize(m_vertexCount, 0);
    for (uint32_t i = 0; i < m_vertexCount; i++) {
        m_sortedIndices[i] = i;
    }
    m_orderSSBO = setupSSBO_int(orderBindIdx, reinterpret_cast<const uint32_t*>(m_sortedIndices.data()), m_vertexCount);
}

// 将float转换为可排序的uint32_t（保持排序顺序）
inline uint32_t floatFlip(float f) {
    uint32_t u;
    std::memcpy(&u, &f, sizeof(float));
    uint32_t mask = -int32_t(u >> 31) | 0x80000000;
    return u ^ mask;
}

// 并行基数排序（针对深度值优化）
void radixSortParallel(std::vector<std::pair<float, uint32_t>>& data) {
    const size_t n = data.size();
    std::vector<std::pair<uint32_t, uint32_t>> buffer(n);
    std::vector<std::pair<uint32_t, uint32_t>> sorted(n);
    
    // 转换float为可排序的uint32（保持排序关系）
    #pragma omp parallel for
    for (int i = 0; i < (int)n; i++) {
        sorted[i] = {floatFlip(data[i].first), data[i].second};
    }
    
    // 8位基数排序，4轮（处理32位）
    for (int shift = 0; shift < 32; shift += 8) {
        size_t count[256] = {0};
        
        // 计数
        for (size_t i = 0; i < n; i++) {
            uint8_t byte = (sorted[i].first >> shift) & 0xFF;
            count[byte]++;
        }
        
        // 前缀和
        size_t total = 0;
        for (int i = 0; i < 256; i++) {
            size_t c = count[i];
            count[i] = total;
            total += c;
        }
        
        // 分配到buffer
        for (size_t i = 0; i < n; i++) {
            uint8_t byte = (sorted[i].first >> shift) & 0xFF;
            buffer[count[byte]++] = sorted[i];
        }
        
        // 交换buffer和sorted
        sorted.swap(buffer);
    }
    
    // 写回结果（只需要索引）
    #pragma omp parallel for
    for (int i = 0; i < (int)n; i++) {
        data[i].second = sorted[i].second;
    }
}

std::vector<std::pair<float, uint32_t>> depthIndices;
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
    depthIndices.clear();
    depthIndices.reserve(m_vertexCount);

    Matrix4 transposedView = view.transpose();
    float v8 = view.m[8];
    float v9 = view.m[9];
    float v10 = view.m[10];
    float v11 = view.m[11];

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    int thread_num = std::thread::hardware_concurrency(); // 使用CPU核心数
    if (thread_num == 0) thread_num = 8;
    
    // 预分配所有空间
    depthIndices.resize(m_vertexCount);
    
    std::vector<std::thread> threads;
    threads.reserve(thread_num);
    
    // 计算每个线程处理的数据范围
    uint32_t chunkSize = (m_vertexCount + thread_num - 1) / thread_num;
    
    for (int t = 0; t < thread_num; t++) {
        threads.emplace_back([&, t, chunkSize]() {
            uint32_t start = t * chunkSize;
            uint32_t end = std::min(start + chunkSize, m_vertexCount);
            
            for (uint32_t i = start; i < end; i++) {
                const float* pos = m_gaussianPoints[i].position;
                float viewZ = v8 * pos[0] + v9 * pos[1] + v10 * pos[2] + v11;
                depthIndices[i] = {viewZ, i};
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }

    // for (uint32_t i = 0; i < m_vertexCount; i++) {
    //     const float* pos = m_gaussianPoints[i].position;
        
    //     // 手动计算 view * position（只需要z分量）
    //     float viewZ = v8 * pos[0] + v9 * pos[1] + v10 * pos[2] + v11;
    //     // Vector3 viewPos = transposedView * Vector3(pos[0], pos[1], pos[2]);
    //     // viewZ = viewPos.z;
    //     depthIndices[i] = {viewZ, i};
    // }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;
    LOG_INFO("计算时间: {}秒", duration.count());

    std::sort(std::execution::par_unseq, depthIndices.begin(), depthIndices.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });
    //radixSortParallel(depthIndices);

    end = std::chrono::steady_clock::now();
    duration = end - start;
    LOG_INFO("排序时间: {}秒", duration.count());
    // // 更新排序后的索引
    for (size_t i = 0; i < depthIndices.size(); i++) {
        m_sortedIndices[i] = depthIndices[i].second;
    }

        // // 调试输出：打印前10个和后10个点的深度
        // LOG_INFO("=== 排序验证 ===");
        // LOG_INFO("前10个（应该最远）:");
        // for (int i = 0; i < std::min(10, (int)depthIndices.size()); i++) {
        //     LOG_INFO("  [{}] 索引:{}, 深度:{:.3f}", i, depthIndices[i].second, depthIndices[i].first);
        // }
        // LOG_INFO("后10个（应该最近）:");
        // for (int i = std::max(0, (int)depthIndices.size()-10); i < depthIndices.size(); i++) {
        //     LOG_INFO("  [{}] 索引:{}, 深度:{:.3f}", i, depthIndices[i].second, depthIndices[i].first);
        // }
        // LOG_INFO("==================");
}

void GaussianRenderer::drawSplats(const Renderer::Matrix4& model, const Renderer::Matrix4& view, 
                                   const Renderer::Matrix4& projection, int width, int height)
{
    // if (m_vertexCount == 0 || m_splatVAO == 0) {
    //     LOG_WARN("drawSplats: 没有数据或VAO未初始化");
    //     return;
    // }

    try {
        GLenum err = glGetError();
        m_frameBuffer->Bind();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("绑定帧缓冲区错误: {}", err);
            return;
        }
        m_frameBuffer->ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        m_frameBuffer->ClearDepthStencil(1.0f, 0);
        err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("清除帧缓冲区错误: {}", err);
            return;
        }
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        static bool isSorted = false;
        static Renderer::Vector3 camPosition;
        if (!isSorted) {
        // 1. 深度排序
        sortGaussiansByDepth(view);

        //LOG_INFO("Camera Position: ({}, {}, {})", camPosition.x, camPosition.y, camPosition.z);
        //isSorted = true;
        }
        // 2. 更新GPU上的排序索引
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_orderSSBO);
        err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("绑定SSBO错误: {}", err);
            return;
        }
        
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 
            m_vertexCount * sizeof(uint32_t), 
            reinterpret_cast<const uint32_t*>(m_sortedIndices.data()));
        
        err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("更新SSBO数据错误: {}", err);
            return;
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        
        // 3. 启用混合
        //glDisable(GL_CULL_FACE);
        //glCullFace(GL_BACK);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);  // 禁用深度写入（但保持深度测试）
        //glEnable(GL_DEPTH_TEST);
        glDisable(GL_DEPTH_TEST); // 启用深度测试
        
        // 4. 计算焦距参数
        float fov = 50.0f * 3.14159265f / 180.0f;
        float aspect = width * 1.0f / height;
        float htany = std::tan(fov / 2.0f);
        float htanx = std::tan(fov / 2.0f) * aspect;
        float focal_x = width / (2.0f * htanx);
        float focal_y = height / (2.0f * htany);
        
        // 5. 使用shader并设置uniform
        m_splatShader.use();
        m_splatShader.setMat4("view", view.data());
        m_splatShader.setMat4("projection", projection.data());
        m_splatShader.setVec4("hfov_focal", htanx, htany, focal_x, focal_y);
        m_splatShader.setFloat("scaleMod", 1.0f);

        Renderer::Matrix4 invViewMatrix = Renderer::Matrix4(view).inverse();
        camPosition = Renderer::Vector3(invViewMatrix.m[12], invViewMatrix.m[13], invViewMatrix.m[14]);
        m_splatShader.setVec3("campos", camPosition.x, camPosition.y, camPosition.z);

        // 6. 绑定VAO
        glBindVertexArray(m_quadVAO);
        err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("绑定VAO错误: {}", err);
            return;
        }
        
        // 7. 限制渲染数量（关键！）
        // 先用小数量测试，避免GPU超时
        // static uint32_t renderCount = 0;
        // renderCount += 10000;
        // if (renderCount > m_vertexCount) {
        //     renderCount = m_vertexCount;
        // }
        uint32_t renderCount = m_vertexCount;//std::min(m_vertexCount, 10000u);  // 先只渲染1万个点测试
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, renderCount);
        // int loop = 0;
        // for (uint32_t i = 0; i < renderCount; i+=1000) {
        //     m_splatShader.setInt("loop", loop);
        //     m_splatShader.setInt("instanceID", i);
        //     glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1000);
        //     loop++;
        // }
        
        err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("绘制错误: {}", err);
            return;
        }

        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glFlush();
        glFinish();
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::duration<double> duration = end - start;
        LOG_INFO("总绘制时间: {}秒", duration.count());
        m_frameBuffer->Unbind();
    } catch (const std::exception& e) {
        LOG_ERROR("drawSplats异常: {}", e.what());
    }
}

RENDERER_NAMESPACE_END