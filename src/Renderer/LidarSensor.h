#pragma once

#include "Core/RenderCore.h"
#include "MathUtils/Vector.h"
#include "Shader.h"
#include "FrameBuffer.h"
#include "Primitives/Primitive.h"
#include "MathUtils/Matrix.h"
#include <vector>

RENDERER_NAMESPACE_BEGIN

class RENDERER_API LidarSensor {
public:
    LidarSensor(Vector3 position, Vector3 direction, float fov, float distance);
    ~LidarSensor();

    void setDirection(const Vector3& direction) { m_direction = direction; }
    void renderDepth(const std::vector<std::pair<Primitive*, Matrix4>> &primitives);

    void renderVisualization(const float* cameraView, const float* cameraProjection, int width, int height, unsigned int sceneDepthTexture);

private:
    void generateSphereMesh();
    void createDepthCubemap();
    void buildCubemapViewMatrices();
private:
    Vector3 m_position;
    Vector3 m_direction;
    float m_fov;
    float m_distance;

    //FrameBuffer m_frameBuffer;
    Shader m_depthShader;
    Shader m_visShader;

    unsigned int m_depthCubemap;
    unsigned int m_depthCubemapFBO;
    int m_depthMapResolution = 512;
    Matrix4 m_cubemapView[6];
    Matrix4 m_cubemapProjection;

    int m_indexCount = 0;

    unsigned int m_vao;
    unsigned int m_vbo;
    unsigned int m_ebo;
};

RENDERER_NAMESPACE_END