#include "GaussianSplatting/GaussianRenderer.h"
#include "FrameBuffer.h"
#include "Logger/Log.h"
#include "MathUtils/Vector.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <execution>
#include <fstream>
#include <glad/glad.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

RENDERER_NAMESPACE_BEGIN

float sigmoid(float x)
{
    return 1.0f / (1.0f + std::exp(-x));
}

unsigned int setupSSBO(const unsigned int &bindIdx, const float *bufferData, const size_t &bufferSize)
{
    unsigned int ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize * sizeof(float), bufferData, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindIdx, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return ssbo;
}

unsigned int setupSSBO_int(const unsigned int &bindIdx, const uint32_t *bufferData, const size_t &bufferSize)
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
      m_splatShader(Renderer::Shader::fromFiles("res/shaders/gaussian.vs.glsl", "res/shaders/gaussian.fs.glsl"))
{

    m_frameBuffer = new FrameBuffer();
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2560, 1440, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_frameBuffer->Attach(FrameBuffer::Attachment::Color0, m_colorTexture);

    // 启动后台排序线程
    m_sortThreadRunning = true;
    m_sortThread = std::thread(&GaussianRenderer::backgroundSortThread, this);

    LOG_INFO("GaussianRenderer创建成功");
}

GaussianRenderer::~GaussianRenderer()
{
    // 停止后台排序线程
    {
        std::lock_guard<std::mutex> lock(m_sortMutex);
        m_shutdownThread = true;
        m_sortRequested = true; // 唤醒线程
    }
    m_sortCV.notify_one();

    if (m_sortThread.joinable())
    {
        m_sortThread.join();
    }

    if (m_vao != 0)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0)
        glDeleteBuffers(1, &m_vbo);
    if (m_splatVAO != 0)
        glDeleteVertexArrays(1, &m_splatVAO);
    if (m_splatVBO != 0)
        glDeleteBuffers(1, &m_splatVBO);
    if (m_quadVBO != 0)
        glDeleteBuffers(1, &m_quadVBO);
    if (m_instanceVBO != 0)
        glDeleteBuffers(1, &m_instanceVBO);
    if (m_orderSSBO != 0)
        glDeleteBuffers(1, &m_orderSSBO);
    if (m_pointsSSBO != 0)
        glDeleteBuffers(1, &m_pointsSSBO);
    if (m_frameBuffer != nullptr)
        delete m_frameBuffer;
    if (m_colorTexture != 0)
        glDeleteTextures(1, &m_colorTexture);
}

void GaussianRenderer::loadModel(const std::string &path)
{
    std::ifstream ifs(path, std::ios_base::binary);
    if (!ifs.is_open())
    {
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

    while (buffer != "end_header")
    {
        std::getline(ifs, buffer);
    }

    m_vertexCount = count;
    m_gaussianPoints.resize(m_vertexCount);
    ifs.read(reinterpret_cast<char *>(m_gaussianPoints.data()), m_vertexCount * sizeof(GaussianPoint<SH_ORDER>));

    // 计算模型的边界框
    float minX = 1e10f, minY = 1e10f, minZ = 1e10f;
    float maxX = -1e10f, maxY = -1e10f, maxZ = -1e10f;

    for (size_t i = 0; i < m_vertexCount; i++)
    {
        m_gaussianPoints[i].position[1] = -m_gaussianPoints[i].position[1];
        m_gaussianPoints[i].position[2] = -m_gaussianPoints[i].position[2];

        // m_gaussianPoints[i].normal[1] = -m_gaussianPoints[i].normal[1];
        // m_gaussianPoints[i].normal[2] = -m_gaussianPoints[i].normal[2];

        // 更新边界框
        minX = std::min(minX, m_gaussianPoints[i].position[0]);
        minY = std::min(minY, m_gaussianPoints[i].position[1]);
        minZ = std::min(minZ, m_gaussianPoints[i].position[2]);
        maxX = std::max(maxX, m_gaussianPoints[i].position[0]);
        maxY = std::max(maxY, m_gaussianPoints[i].position[1]);
        maxZ = std::max(maxZ, m_gaussianPoints[i].position[2]);

        // 旋转归一化
        float angle = std::sqrt(m_gaussianPoints[i].rotation[0] * m_gaussianPoints[i].rotation[0] +
                                m_gaussianPoints[i].rotation[1] * m_gaussianPoints[i].rotation[1] +
                                m_gaussianPoints[i].rotation[2] * m_gaussianPoints[i].rotation[2] +
                                m_gaussianPoints[i].rotation[3] * m_gaussianPoints[i].rotation[3]);

        if (angle > 0.00001f)
        {
            m_gaussianPoints[i].rotation[0] = m_gaussianPoints[i].rotation[0] / angle;
            m_gaussianPoints[i].rotation[1] = m_gaussianPoints[i].rotation[1] / angle;
            m_gaussianPoints[i].rotation[2] = m_gaussianPoints[i].rotation[2] / angle;
            m_gaussianPoints[i].rotation[3] = m_gaussianPoints[i].rotation[3] / angle;

            // 对于wxyz顺序的四元数，绕X轴旋转180度的变换：
            // q' = q_rotateX * q_original
            // 其中 q_rotateX = (0, 1, 0, 0) 表示绕X轴旋转180度

            float w_orig = m_gaussianPoints[i].rotation[0]; // w
            float x_orig = m_gaussianPoints[i].rotation[1]; // x
            float y_orig = m_gaussianPoints[i].rotation[2]; // y
            float z_orig = m_gaussianPoints[i].rotation[3]; // z

            // 应用变换：q' = q_rotateX * q_original
            // 对于绕X轴旋转180度，这相当于：
            // w' = -x_orig
            // x' = w_orig
            // y' = -z_orig
            // z' = y_orig
            float w_new = -x_orig;
            float x_new = w_orig;
            float y_new = -z_orig;
            float z_new = y_orig;

            m_gaussianPoints[i].rotation[0] = w_new;
            m_gaussianPoints[i].rotation[1] = x_new;
            m_gaussianPoints[i].rotation[2] = y_new;
            m_gaussianPoints[i].rotation[3] = z_new;
        }
        else
        {
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

    // 执行Morton排序以提高空间局部性
    mortonSort();

    setupBuffers(); // 点云渲染需要这个
    setupSplatBuffers();

    float centerX = (minX + maxX) * 0.5f;
    float centerY = (minY + maxY) * 0.5f;
    float centerZ = (minZ + maxZ) * 0.5f;
    float size = std::max({maxX - minX, maxY - minY, maxZ - minZ});

    LOG_INFO("=== 高斯模型信息 ===");
    LOG_INFO("点数量: {}", m_vertexCount);
    LOG_INFO("边界: X[{:.2f}, {:.2f}], Y[{:.2f}, {:.2f}], Z[{:.2f}, {:.2f}]", minX, maxX, minY, maxY, minZ, maxZ);
    LOG_INFO("中心: ({:.2f}, {:.2f}, {:.2f})", centerX, centerY, centerZ);
    LOG_INFO("最大尺寸: {:.2f}", size);
    LOG_INFO("建议相机位置: ({:.2f}, {:.2f}, {:.2f})", centerX, centerY, centerZ + size * 1.5f);
    LOG_INFO("==================");
}

void GaussianRenderer::drawPoints(const Renderer::Matrix4 &model, const Renderer::Matrix4 &view,
                                  const Renderer::Matrix4 &projection)
{
    m_frameBuffer->Bind();
    m_frameBuffer->ClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    m_shader.use();
    m_shader.setMat4("model", model.data());
    m_shader.setMat4("view", view.data());
    m_shader.setMat4("projection", projection.data());

    // Matrix4 transposedView = view.transpose();
    // transposedView = transposedView.inverse();
    // float v8 = transposedView.m[3];
    // float v9 = transposedView.m[7];
    // float v10 = transposedView.m[11];
    // float v11 = transposedView.m[15];
    // LOG_INFO("Camera Position: ({}, {}, {})", v8, v9, v10);
    glPointSize(3.0f);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_POINTS, 0, m_vertexCount);
    glBindVertexArray(0);
    m_frameBuffer->Unbind();
}

// Morton码编码（21位/轴，总63位，与官方实现一致）
uint64_t encodeMorton3D21(float x, float y, float z)
{
    // 量化到[0, 2^21 - 1]
    constexpr uint32_t kMax = (1u << 21) - 1u;
    auto quantize = [kMax](float v) -> uint32_t {
        float q = std::clamp(v, 0.0f, 1.0f) * static_cast<float>(kMax);
        return static_cast<uint32_t>(q + 0.5f); // 四舍五入
    };

    uint32_t ix = quantize(x);
    uint32_t iy = quantize(y);
    uint32_t iz = quantize(z);

    // 逐位交织
    uint64_t code = 0;
    for (int i = 0; i < 21; ++i)
    {
        code |= (uint64_t(ix & (1u << i)) << (2 * i + 0));
        code |= (uint64_t(iy & (1u << i)) << (2 * i + 1));
        code |= (uint64_t(iz & (1u << i)) << (2 * i + 2));
    }
    return code;
}

void GaussianRenderer::mortonSort()
{
    if (m_gaussianPoints.empty())
    {
        return;
    }

    // 计算边界框
    float minX = 1e10f, minY = 1e10f, minZ = 1e10f;
    float maxX = -1e10f, maxY = -1e10f, maxZ = -1e10f;

    for (const auto &point : m_gaussianPoints)
    {
        minX = std::min(minX, point.position[0]);
        minY = std::min(minY, point.position[1]);
        minZ = std::min(minZ, point.position[2]);
        maxX = std::max(maxX, point.position[0]);
        maxY = std::max(maxY, point.position[1]);
        maxZ = std::max(maxZ, point.position[2]);
    }

    // 计算范围
    float rangeX = maxX - minX;
    float rangeY = maxY - minY;
    float rangeZ = maxZ - minZ;

    // 避免除零错误
    if (rangeX < 1e-6f)
        rangeX = 1.0f;
    if (rangeY < 1e-6f)
        rangeY = 1.0f;
    if (rangeZ < 1e-6f)
        rangeZ = 1.0f;

    // 创建包含Morton码和原始索引的向量
    struct MortonIndex
    {
        uint64_t mortonCode;
        uint32_t originalIndex;
    };

    std::vector<MortonIndex> mortonIndices;
    mortonIndices.reserve(m_vertexCount);

    // 为每个点计算Morton码
    for (uint32_t i = 0; i < m_vertexCount; ++i)
    {
        // 归一化坐标到[0,1]范围
        float nx = (m_gaussianPoints[i].position[0] - minX) / rangeX;
        float ny = (m_gaussianPoints[i].position[1] - minY) / rangeY;
        float nz = (m_gaussianPoints[i].position[2] - minZ) / rangeZ;

        uint64_t mortonCode = encodeMorton3D21(nx, ny, nz);
        mortonIndices.push_back({mortonCode, i});
    }

    // 按Morton码排序
    std::sort(mortonIndices.begin(), mortonIndices.end(),
              [](const MortonIndex &a, const MortonIndex &b) { return a.mortonCode < b.mortonCode; });

    // 创建排序后的索引数组
    m_sortedIndices.resize(m_vertexCount);
    for (uint32_t i = 0; i < m_vertexCount; ++i)
    {
        m_sortedIndices[i] = mortonIndices[i].originalIndex;
    }

    // 根据Morton顺序重排实际数据，保证CPU/GPU访问的局部性
    std::vector<GaussianPoint<SH_ORDER>> sortedPoints(m_vertexCount);
    for (uint32_t i = 0; i < m_vertexCount; ++i)
    {
        sortedPoints[i] = m_gaussianPoints[m_sortedIndices[i]];
        m_sortedIndices[i] = i; // 重排后索引与数据顺序一致
    }
    m_gaussianPoints.swap(sortedPoints);

    LOG_INFO("Morton排序完成，共{}个点", m_vertexCount);
}

void GaussianRenderer::setupBuffers()
{
    if (m_vao != 0)
    {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo != 0)
    {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    std::cout << "Size of GaussianPoint<" << SH_ORDER << ">: " << sizeof(GaussianPoint<SH_ORDER>) << std::endl;
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertexCount * sizeof(GaussianPoint<SH_ORDER>), m_gaussianPoints.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GaussianPoint<SH_ORDER>),
                          (void *)offsetof(GaussianPoint<SH_ORDER>, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GaussianPoint<SH_ORDER>),
                          (void *)offsetof(GaussianPoint<SH_ORDER>, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(GaussianPoint<SH_ORDER>),
                          (void *)offsetof(GaussianPoint<SH_ORDER>, shs));

    glBindVertexArray(0);
}

void GaussianRenderer::setupSplatBuffers()
{
    // 清理旧的缓冲区
    if (m_splatVAO != 0)
        glDeleteVertexArrays(1, &m_splatVAO);
    if (m_splatVBO != 0)
        glDeleteBuffers(1, &m_splatVBO);
    if (m_quadVBO != 0)
        glDeleteBuffers(1, &m_quadVBO);
    if (m_instanceVBO != 0)
        glDeleteBuffers(1, &m_instanceVBO);
    if (m_quadVAO != 0)
        glDeleteVertexArrays(1, &m_quadVAO);

    float quadVertices[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};

    glGenVertexArrays(1, &m_quadVAO);
    glBindVertexArray(m_quadVAO);

    glGenBuffers(1, &m_quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint pointsBindIdx = 1;
    m_pointsSSBO = setupSSBO(pointsBindIdx, reinterpret_cast<const float *>(m_gaussianPoints.data()),
                             m_vertexCount * sizeof(GaussianPoint<SH_ORDER>) / sizeof(float));

    GLuint orderBindIdx = 2;
    // 如果尚未生成排序索引，则使用顺序索引
    if (m_sortedIndices.size() != m_vertexCount)
    {
        m_sortedIndices.resize(m_vertexCount);
        for (uint32_t i = 0; i < m_vertexCount; i++)
        {
            m_sortedIndices[i] = i;
        }
    }
    m_orderSSBO =
        setupSSBO_int(orderBindIdx, reinterpret_cast<const uint32_t *>(m_sortedIndices.data()), m_vertexCount);
}

// 将float转换为可排序的uint32_t（保持排序顺序）
inline uint32_t floatFlip(float f)
{
    uint32_t u;
    std::memcpy(&u, &f, sizeof(float));
    uint32_t mask = -int32_t(u >> 31) | 0x80000000;
    return u ^ mask;
}

// 并行基数排序（针对深度值优化）
void radixSortParallel(std::vector<std::pair<float, uint32_t>> &data)
{
    const size_t n = data.size();
    std::vector<std::pair<uint32_t, uint32_t>> buffer(n);
    std::vector<std::pair<uint32_t, uint32_t>> sorted(n);

// 转换float为可排序的uint32（保持排序关系）
#pragma omp parallel for
    for (int i = 0; i < (int)n; i++)
    {
        sorted[i] = {floatFlip(data[i].first), data[i].second};
    }

    // 8位基数排序，4轮（处理32位）
    for (int shift = 0; shift < 32; shift += 8)
    {
        size_t count[256] = {0};

        // 计数
        for (size_t i = 0; i < n; i++)
        {
            uint8_t byte = (sorted[i].first >> shift) & 0xFF;
            count[byte]++;
        }

        // 前缀和
        size_t total = 0;
        for (int i = 0; i < 256; i++)
        {
            size_t c = count[i];
            count[i] = total;
            total += c;
        }

        // 分配到buffer
        for (size_t i = 0; i < n; i++)
        {
            uint8_t byte = (sorted[i].first >> shift) & 0xFF;
            buffer[count[byte]++] = sorted[i];
        }

        // 交换buffer和sorted
        sorted.swap(buffer);
    }

// 写回结果（只需要索引）
#pragma omp parallel for
    for (int i = 0; i < (int)n; i++)
    {
        data[i].second = sorted[i].second;
    }
}

std::vector<std::pair<float, uint32_t>> depthIndices;

// 直方图排序的辅助数据结构
struct ChunkBound
{
    float centerX, centerY, centerZ, radius;
};

struct SceneBounds
{
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
    bool initialized = false;
};

std::vector<ChunkBound> chunks;
std::vector<uint32_t> distances;
std::vector<uint32_t> countBuffer;
SceneBounds sceneBounds;

void GaussianRenderer::sortGaussiansByDepthHistogram(const Renderer::Matrix4 &view)
{
    if (m_gaussianPoints.empty() || m_vertexCount == 0)
        return;

    std::chrono::steady_clock::time_point totalStart = std::chrono::steady_clock::now();

    // 1. 提取相机位置和方向
    Matrix4 invView = view.inverse();
    float px = invView.m[12];
    float py = invView.m[13];
    float pz = invView.m[14];

    LOG_INFO("相机位置: ({}, {}, {})", px, py, pz);

    // 相机前向方向（view矩阵的第3列取反）
    Matrix4 transposedView = view.transpose();
    float dx = transposedView.m[8];
    float dy = transposedView.m[9];
    float dz = transposedView.m[10];

    LOG_INFO("相机前向方向: ({}, {}, {})", dx, dy, dz);

    // 2. 初始化场景边界（只计算一次）
    if (!sceneBounds.initialized)
    {
        sceneBounds.minX = sceneBounds.minY = sceneBounds.minZ = 1e10f;
        sceneBounds.maxX = sceneBounds.maxY = sceneBounds.maxZ = -1e10f;

        for (uint32_t i = 0; i < m_vertexCount; ++i)
        {
            const float *pos = m_gaussianPoints[i].position;
            sceneBounds.minX = std::min(sceneBounds.minX, pos[0]);
            sceneBounds.maxX = std::max(sceneBounds.maxX, pos[0]);
            sceneBounds.minY = std::min(sceneBounds.minY, pos[1]);
            sceneBounds.maxY = std::max(sceneBounds.maxY, pos[1]);
            sceneBounds.minZ = std::min(sceneBounds.minZ, pos[2]);
            sceneBounds.maxZ = std::max(sceneBounds.maxZ, pos[2]);
        }
        sceneBounds.initialized = true;
        LOG_INFO("场景边界初始化完成");
    }

    // 3. 计算边界框8个角点到相机方向的距离范围
    float minDist = 1e10f, maxDist = -1e10f;
    for (int i = 0; i < 8; ++i)
    {
        float x = (i & 1) ? sceneBounds.minX : sceneBounds.maxX;
        float y = (i & 2) ? sceneBounds.minY : sceneBounds.maxY;
        float z = (i & 4) ? sceneBounds.minZ : sceneBounds.maxZ;
        float d = x * dx + y * dy + z * dz;
        minDist = std::min(minDist, d);
        maxDist = std::max(maxDist, d);
    }

    const float range = maxDist - minDist;

    // 4. 计算排序需要的位数（使用更少的位数以提高性能）
    const uint32_t compareBits = std::max(12u, std::min(16u, uint32_t(std::round(std::log2(m_vertexCount / 4)))));
    const uint32_t bucketCount = (1u << compareBits);
    LOG_INFO("排序需要的位数: {}", compareBits);
    LOG_INFO("桶数量: {}", bucketCount);

    // 5. 初始化缓冲区
    if (distances.size() != m_vertexCount)
        distances.resize(m_vertexCount);

    if (countBuffer.size() != bucketCount)
    {
        countBuffer.resize(bucketCount);
    }
    std::fill(countBuffer.begin(), countBuffer.end(), 0);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    // 6. 使用手动线程池并行计算距离并映射到桶
    int thread_num = std::thread::hardware_concurrency();
    if (thread_num == 0)
        thread_num = 8;

    const float scale = (range > 1e-6f) ? (float(bucketCount - 1) / range) : 0.0f;
    LOG_INFO("比例: {}", scale);

    std::vector<std::thread> threads;
    threads.reserve(thread_num);

    uint32_t chunkSize = (m_vertexCount + thread_num - 1) / thread_num;

    for (int t = 0; t < thread_num; ++t)
    {
        threads.emplace_back([&, t, chunkSize, scale]() {
            uint32_t startIdx = t * chunkSize;
            uint32_t endIdx = std::min(startIdx + chunkSize, m_vertexCount);

            for (uint32_t i = startIdx; i < endIdx; ++i)
            {
                const float *pos = m_gaussianPoints[i].position;
                float d = pos[0] * dx + pos[1] * dy + pos[2] * dz - minDist;
                uint32_t sortKey = (range > 1e-6f) ? std::min(bucketCount - 1, uint32_t(d * scale)) : 0u;
                distances[i] = sortKey;
            }
        });
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;
    LOG_INFO("距离计算时间: {:.3f} ms", duration.count() * 1000.0);

    // 7. 计数排序（优化版）
    start = std::chrono::steady_clock::now();

    // 并行计数 - 使用局部计数器避免竞争
    std::vector<std::vector<uint32_t>> localCounts(thread_num, std::vector<uint32_t>(bucketCount, 0));

    threads.clear();
    for (int t = 0; t < thread_num; ++t)
    {
        threads.emplace_back([&, t, chunkSize]() {
            uint32_t startIdx = t * chunkSize;
            uint32_t endIdx = std::min(startIdx + chunkSize, m_vertexCount);

            for (uint32_t i = startIdx; i < endIdx; ++i)
            {
                localCounts[t][distances[i]]++;
            }
        });
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    // 合并计数
    for (int t = 0; t < thread_num; ++t)
    {
        for (uint32_t i = 0; i < bucketCount; ++i)
        {
            countBuffer[i] += localCounts[t][i];
        }
    }

    // 转换为前缀和
    for (uint32_t i = 1; i < bucketCount; ++i)
    {
        countBuffer[i] += countBuffer[i - 1];
    }

    // 确保排序索引已分配
    if (m_sortedIndices.size() != m_vertexCount)
        m_sortedIndices.resize(m_vertexCount);

    // 从后往前构建输出数组（稳定排序）
    for (int32_t i = m_vertexCount - 1; i >= 0; --i)
    {
        uint32_t distance = distances[i];
        uint32_t destIndex = --countBuffer[distance];
        m_sortedIndices[destIndex] = i;
    }

    end = std::chrono::steady_clock::now();
    duration = end - start;
    LOG_INFO("计数排序时间: {:.3f} ms", duration.count() * 1000.0);

    std::chrono::steady_clock::time_point totalEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> totalDuration = totalEnd - totalStart;
    LOG_INFO("直方图排序总时间: {:.3f} ms", totalDuration.count() * 1000.0);
}

// 后台排序线程
void GaussianRenderer::backgroundSortThread()
{
    while (m_sortThreadRunning)
    {
        std::unique_lock<std::mutex> lock(m_sortMutex);

        // 等待排序请求
        m_sortCV.wait(lock, [this] { return m_sortRequested.load() || m_shutdownThread.load(); });

        if (m_shutdownThread)
        {
            break;
        }

        if (m_sortRequested)
        {
            m_sortRequested = false;
            m_isSorting = true; // 标记开始排序
            Renderer::Matrix4 viewMatrix = m_pendingViewMatrix;
            lock.unlock(); // 解锁以便主线程继续运行

            // 执行排序（在后台线程中）
            if (m_gaussianPoints.empty() || m_vertexCount == 0)
            {
                m_isSorting = false;
                continue;
            }

            std::chrono::steady_clock::time_point sortStart = std::chrono::steady_clock::now();

            // 使用直方图排序（与sortGaussiansByDepthHistogram类似，但写入buffer）
            Matrix4 invView = viewMatrix.inverse();
            float px = invView.m[12];
            float py = invView.m[13];
            float pz = invView.m[14];

            Matrix4 transposedView = viewMatrix.transpose();
            float dx = transposedView.m[8];
            float dy = transposedView.m[9];
            float dz = transposedView.m[10];

            // 计算边界距离
            if (!sceneBounds.initialized)
            {
                sceneBounds.minX = sceneBounds.minY = sceneBounds.minZ = 1e10f;
                sceneBounds.maxX = sceneBounds.maxY = sceneBounds.maxZ = -1e10f;

                for (uint32_t i = 0; i < m_vertexCount; ++i)
                {
                    const float *pos = m_gaussianPoints[i].position;
                    sceneBounds.minX = std::min(sceneBounds.minX, pos[0]);
                    sceneBounds.maxX = std::max(sceneBounds.maxX, pos[0]);
                    sceneBounds.minY = std::min(sceneBounds.minY, pos[1]);
                    sceneBounds.maxY = std::max(sceneBounds.maxY, pos[1]);
                    sceneBounds.minZ = std::min(sceneBounds.minZ, pos[2]);
                    sceneBounds.maxZ = std::max(sceneBounds.maxZ, pos[2]);
                }
                sceneBounds.initialized = true;
            }

            float minDist = 1e10f, maxDist = -1e10f;
            for (int i = 0; i < 8; ++i)
            {
                float x = (i & 1) ? sceneBounds.minX : sceneBounds.maxX;
                float y = (i & 2) ? sceneBounds.minY : sceneBounds.maxY;
                float z = (i & 4) ? sceneBounds.minZ : sceneBounds.maxZ;
                float d = x * dx + y * dy + z * dz;
                minDist = std::min(minDist, d);
                maxDist = std::max(maxDist, d);
            }

            const float range = maxDist - minDist;
            const uint32_t compareBits = std::max(12u, std::min(16u, uint32_t(std::round(std::log2(m_vertexCount)))));
            const uint32_t bucketCount = (1u << compareBits);

            if (distances.size() != m_vertexCount)
                distances.resize(m_vertexCount);

            if (countBuffer.size() != bucketCount)
                countBuffer.resize(bucketCount);
            std::fill(countBuffer.begin(), countBuffer.end(), 0);

            // 并行计算距离
            const float scale = (range > 1e-6f) ? (float(bucketCount - 1) / range) : 0.0f;

            int thread_num = std::thread::hardware_concurrency();
            if (thread_num == 0)
                thread_num = 8;

            std::vector<std::thread> threads;
            threads.reserve(thread_num);

            uint32_t chunkSize = (m_vertexCount + thread_num - 1) / thread_num;

            for (int t = 0; t < thread_num; ++t)
            {
                threads.emplace_back([&, t, chunkSize, scale]() {
                    uint32_t startIdx = t * chunkSize;
                    uint32_t endIdx = std::min(startIdx + chunkSize, m_vertexCount);

                    for (uint32_t i = startIdx; i < endIdx; ++i)
                    {
                        const float *pos = m_gaussianPoints[i].position;
                        float d = pos[0] * dx + pos[1] * dy + pos[2] * dz - minDist;
                        uint32_t sortKey = (range > 1e-6f) ? std::min(bucketCount - 1, uint32_t(d * scale)) : 0u;
                        distances[i] = sortKey;
                    }
                });
            }

            for (auto &thread : threads)
            {
                thread.join();
            }

            // 计数排序
            std::vector<std::vector<uint32_t>> localCounts(thread_num, std::vector<uint32_t>(bucketCount, 0));

            threads.clear();
            for (int t = 0; t < thread_num; ++t)
            {
                threads.emplace_back([&, t, chunkSize]() {
                    uint32_t startIdx = t * chunkSize;
                    uint32_t endIdx = std::min(startIdx + chunkSize, m_vertexCount);

                    for (uint32_t i = startIdx; i < endIdx; ++i)
                    {
                        localCounts[t][distances[i]]++;
                    }
                });
            }

            for (auto &thread : threads)
            {
                thread.join();
            }

            // 合并计数
            for (int t = 0; t < thread_num; ++t)
            {
                for (uint32_t i = 0; i < bucketCount; ++i)
                {
                    countBuffer[i] += localCounts[t][i];
                }
            }

            // 前缀和
            for (uint32_t i = 1; i < bucketCount; ++i)
            {
                countBuffer[i] += countBuffer[i - 1];
            }

            // 写入到buffer
            if (m_sortedIndicesBuffer.size() != m_vertexCount)
                m_sortedIndicesBuffer.resize(m_vertexCount);

            for (int32_t i = m_vertexCount - 1; i >= 0; --i)
            {
                uint32_t distance = distances[i];
                uint32_t destIndex = --countBuffer[distance];
                m_sortedIndicesBuffer[destIndex] = i;
            }

            std::chrono::steady_clock::time_point sortEnd = std::chrono::steady_clock::now();
            std::chrono::duration<double> sortDuration = sortEnd - sortStart;
            LOG_INFO("后台排序完成，耗时: {:.3f} ms", sortDuration.count() * 1000.0);

            // 标记排序完成
            m_isSorting = false;
            m_sortCompleted = true;
        }
    }
}

// 启动后台排序
void GaussianRenderer::startBackgroundSort(const Renderer::Matrix4 &view)
{
    // 如果正在排序，跳过本次请求（避免排序请求堆积）
    if (m_isSorting.load())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_sortMutex);
    m_pendingViewMatrix = view;
    m_sortRequested = true;
    m_sortCV.notify_one();
}

// 更新GPU buffer（在主线程中调用）
void GaussianRenderer::updateGPUBufferIfReady()
{
    if (m_sortCompleted.exchange(false)) // 原子交换，确保只更新一次
    {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        // 交换buffer
        m_sortedIndices.swap(m_sortedIndicesBuffer);

        // 更新GPU
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_orderSSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, m_vertexCount * sizeof(uint32_t),
                        reinterpret_cast<const uint32_t *>(m_sortedIndices.data()));
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::duration<double> duration = end - start;
        LOG_INFO("更新GPU buffer时间: {:.3f} ms", duration.count() * 1000.0);
    }
}

void GaussianRenderer::sortGaussiansByDepth(const Renderer::Matrix4 &view)
{
    // 创建或更新排序索引
    if (m_sortedIndices.size() != m_vertexCount)
    {
        m_sortedIndices.resize(m_vertexCount);
        for (uint32_t i = 0; i < m_vertexCount; i++)
        {
            m_sortedIndices[i] = i;
        }
    }

    // // 计算每个高斯在视图空间的深度，并排序
    // depthIndices.clear();
    // depthIndices.reserve(m_vertexCount);

    Matrix4 transposedView = view.transpose();
    float v8 = transposedView.m[8];
    float v9 = transposedView.m[9];
    float v10 = transposedView.m[10];
    float v11 = transposedView.m[11];

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    int thread_num = std::thread::hardware_concurrency(); // 使用CPU核心数
    if (thread_num == 0)
        thread_num = 8;

    // 预分配所有空间
    if (depthIndices.size() != m_vertexCount)
    {
        depthIndices.resize(m_vertexCount);
    }

    std::vector<std::thread> threads;
    threads.reserve(thread_num);

    // 计算每个线程处理的数据范围
    uint32_t chunkSize = (m_vertexCount + thread_num - 1) / thread_num;

    for (int t = 0; t < thread_num; t++)
    {
        threads.emplace_back([&, t, chunkSize]() {
            uint32_t start = t * chunkSize;
            uint32_t end = std::min(start + chunkSize, m_vertexCount);

            for (uint32_t i = start; i < end; i++)
            {
                const float *pos = m_gaussianPoints[i].position;
                float viewZ = v8 * pos[0] + v9 * pos[1] + v10 * pos[2] + v11;
                depthIndices[i].first = viewZ;
                depthIndices[i].second = i;
            }
        });
    }

    for (auto &thread : threads)
    {
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
    LOG_INFO("计算时间: {:.3f} ms", duration.count() * 1000.0f);

    start = std::chrono::steady_clock::now();
    std::sort(std::execution::par_unseq, depthIndices.begin(), depthIndices.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    // radixSortParallel(depthIndices);

    end = std::chrono::steady_clock::now();
    duration = end - start;
    LOG_INFO("排序时间: {:.3f} ms", duration.count() * 1000.0f);
    // // 更新排序后的索引
    for (size_t i = 0; i < depthIndices.size(); i++)
    {
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

void GaussianRenderer::drawSplats(const Renderer::Matrix4 &model, const Renderer::Matrix4 &view,
                                  const Renderer::Matrix4 &projection, bool useSort, int width, int height,
                                  unsigned int sceneDepthTexture, const Renderer::Vector3 &selectBoxPos,
                                  const Renderer::Vector3 &selectBoxSize, bool deleteSelectPoints,
                                  const Renderer::Vector3 &selectColor, float gaussianScale)
{
    // if (m_vertexCount == 0 || m_splatVAO == 0) {
    //     LOG_WARN("drawSplats: 没有数据或VAO未初始化");
    //     return;
    // }

    try
    {
        GLenum err = glGetError();
        m_frameBuffer->Bind();
        if (err != GL_NO_ERROR)
        {
            LOG_ERROR("绑定帧缓冲区错误: {}", err);
            return;
        }
        m_frameBuffer->ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        m_frameBuffer->ClearDepthStencil(1.0f, 0);
        err = glGetError();
        if (err != GL_NO_ERROR)
        {
            LOG_ERROR("清除帧缓冲区错误: {}", err);
            return;
        }
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        static Renderer::Vector3 camPosition;
        if (useSort)
        {
            // 使用后台排序系统
            static bool useBackgroundSort = false; // 改为false使用前台阻塞式排序

            if (useBackgroundSort)
            {
                // 1. 启动后台排序（非阻塞）
                startBackgroundSort(view);

                // 2. 检查并更新GPU buffer（如果后台排序完成）
                updateGPUBufferIfReady();
            }
            else
            {
                // 前台阻塞式排序（用于对比）
                LOG_INFO("=== 使用前台排序 ===");
                sortGaussiansByDepthHistogram(view);

                std::chrono::steady_clock::time_point end2 = std::chrono::steady_clock::now();
                std::chrono::duration<double> duration = end2 - start;
                LOG_INFO("前台排序总时间: {:.3f} ms", duration.count() * 1000.0f);

                // 更新GPU上的排序索引
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_orderSSBO);
                err = glGetError();
                if (err != GL_NO_ERROR)
                {
                    LOG_ERROR("绑定SSBO错误: {}", err);
                    return;
                }

                std::chrono::steady_clock::time_point start2 = std::chrono::steady_clock::now();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, m_vertexCount * sizeof(uint32_t),
                                reinterpret_cast<const uint32_t *>(m_sortedIndices.data()));

                glFlush();
                glFinish();
                end2 = std::chrono::steady_clock::now();
                duration = end2 - start2;
                LOG_INFO("更新SSBO数据时间: {:.3f} ms", duration.count() * 1000.0f);
                err = glGetError();
                if (err != GL_NO_ERROR)
                {
                    LOG_ERROR("更新SSBO数据错误: {}", err);
                    return;
                }
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            }
        }

        // 3. 启用混合
        // glDisable(GL_CULL_FACE);
        // glCullFace(GL_BACK);
        glEnable(GL_BLEND);
        // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, // RGB: src*alpha + dst*(1-alpha)
                            GL_ONE, GL_ONE_MINUS_SRC_ALPHA        // Alpha: src + dst*(1-alpha)
        );
        glDepthMask(GL_FALSE); // 禁用深度写入（但保持深度测试）
        // glEnable(GL_DEPTH_TEST);
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
        m_splatShader.setFloat("scaleMod", gaussianScale);

        Renderer::Matrix4 invViewMatrix = Renderer::Matrix4(view).inverse();
        camPosition = Renderer::Vector3(invViewMatrix.m[12], invViewMatrix.m[13], invViewMatrix.m[14]);
        m_splatShader.setVec3("campos", camPosition.x, camPosition.y, camPosition.z);

        if (sceneDepthTexture != 0)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sceneDepthTexture);
            m_splatShader.setInt("u_solidDepthTexture", 0);
            m_splatShader.setInt("u_useSolidDepth", 1);
            m_splatShader.setVec2("u_screenSize", (float)width, (float)height);
            m_splatShader.setVec3("u_selectBoxPos", selectBoxPos.x, selectBoxPos.y, selectBoxPos.z);
            m_splatShader.setVec3("u_selectBoxSize", selectBoxSize.x, selectBoxSize.y, selectBoxSize.z);
            m_splatShader.setInt("u_deleteSelectPoints", deleteSelectPoints ? 1 : 0);
            m_splatShader.setVec3("u_selectColor", selectColor.x, selectColor.y, selectColor.z);
        }
        else
        {
            m_splatShader.setInt("u_useSolidDepth", 0);
        }

        // 6. 绑定VAO
        glBindVertexArray(m_quadVAO);
        err = glGetError();
        if (err != GL_NO_ERROR)
        {
            LOG_ERROR("绑定VAO错误: {}", err);
            return;
        }

        // 7. 限制渲染数量（关键！）
        // 先用小数量测试，避免GPU超时
        // static uint32_t renderCount = 0;
        // renderCount += 10000;
        // if (renderCount > m_vertexCount)
        // {
        //     renderCount = 10000; // m_vertexCount;
        // }
        glFlush();
        glFinish();
        uint32_t renderCount = m_vertexCount; // std::min(m_vertexCount, 10000u);  // 先只渲染1万个点测试
        std::chrono::steady_clock::time_point start3 = std::chrono::steady_clock::now();
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, renderCount);
        glFlush();
        glFinish();
        std::chrono::steady_clock::time_point end3 = std::chrono::steady_clock::now();
        std::chrono::duration<double> duration = end3 - start3;
        LOG_INFO("绘制时间: {:.3f} ms", duration.count() * 1000.0f);
        // int loop = 0;
        // for (uint32_t i = 0; i < renderCount; i += 1000)
        // {
        //     m_splatShader.setInt("loop", loop);
        //     m_splatShader.setInt("instanceID", i);
        //     glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1000);
        //     loop++;
        // }

        err = glGetError();
        if (err != GL_NO_ERROR)
        {
            LOG_ERROR("绘制错误: {}", err);
            return;
        }

        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        // glFlush();
        // glFinish();
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        duration = end - start;
        LOG_INFO("总耗时: {:.3f} ms", duration.count() * 1000.0f);
        m_frameBuffer->Unbind();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("drawSplats异常: {}", e.what());
    }
}

RENDERER_NAMESPACE_END
