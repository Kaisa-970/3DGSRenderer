#pragma once

#include "Renderer/FrameBuffer.h"
#include "Renderer/MathUtils/Matrix.h"
#include "Renderer/Shader.h"
#include "TypeDef.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

RENDERER_NAMESPACE_BEGIN

static constexpr int SH_ORDER = 0;

class RENDERER_API GaussianRenderer
{
public:
    GaussianRenderer();
    ~GaussianRenderer();

    void loadModel(const std::string &path, bool standardFormat = false);
    void drawPoints(const Renderer::Matrix4 &model, const Renderer::Matrix4 &view, const Renderer::Matrix4 &projection);

    // 高质量splat渲染
    void drawSplats(const Renderer::Matrix4 &model, const Renderer::Matrix4 &view, const Renderer::Matrix4 &projection,
                    bool useSort, int width, int height, unsigned int sceneDepthTexture = 0,
                    const Renderer::Vector3 &selectBoxPos = Renderer::Vector3(0.0f, 0.0f, 0.0f),
                    const Renderer::Vector3 &selectBoxSize = Renderer::Vector3(1.0f, 1.0f, 1.0f),
                    bool deleteSelectPoints = false,
                    const Renderer::Vector3 &selectColor = Renderer::Vector3(0.5f, 0.5f, 0.0f),
                    float gaussianScale = 1.0f);

    unsigned int getColorTexture() const
    {
        return m_colorTexture;
    }

private:
    void mortonSort();
    void setupBuffers();
    void setupSplatBuffers();
    void sortGaussiansByDepth(const Renderer::Matrix4 &view);          // 原始std::sort方法
    void sortGaussiansByDepthHistogram(const Renderer::Matrix4 &view); // 直方图+计数排序（最快）

    // 后台排序相关
    void backgroundSortThread();
    void startBackgroundSort(const Renderer::Matrix4 &view);
    void updateGPUBufferIfReady();

private:
    std::vector<NormalPoint> m_points;
    std::vector<GaussianPoint<SH_ORDER>> m_gaussianPoints;
    std::vector<uint32_t> m_sortedIndices; // 排序后的索引

    // 后台排序系统
    std::thread m_sortThread;
    std::mutex m_sortMutex;
    std::condition_variable m_sortCV;
    std::atomic<bool> m_sortThreadRunning{false};
    std::atomic<bool> m_sortRequested{false};
    std::atomic<bool> m_sortCompleted{false};
    std::atomic<bool> m_shutdownThread{false};
    std::atomic<bool> m_isSorting{false}; // 是否正在排序

    Renderer::Matrix4 m_pendingViewMatrix;
    std::vector<uint32_t> m_sortedIndicesBuffer; // 双缓冲：后台线程写入这里

    Renderer::Shader m_shader;
    Renderer::Shader m_splatShader;

    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_vertexCount = 0;

    // Splat渲染的VAO和VBO
    unsigned int m_splatVAO = 0;
    unsigned int m_splatVBO = 0;
    unsigned int m_quadVAO = 0;     // 高斯数据
    unsigned int m_quadVBO = 0;     // 四边形顶点数据
    unsigned int m_instanceVBO = 0; // 实例数据（排序后的索引）
    unsigned int m_orderSSBO = 0;
    unsigned int m_pointsSSBO = 0;

    FrameBuffer *m_frameBuffer = nullptr; // 帧缓冲区
    unsigned int m_colorTexture = 0;      // 颜色纹理
};

RENDERER_NAMESPACE_END
