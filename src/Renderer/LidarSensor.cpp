#include "LidarSensor.h"
#include <glad/glad.h>
#include <cmath>
#include "Logger/Log.h"

RENDERER_NAMESPACE_BEGIN

static const float PI = 3.14159265358979323846f;

LidarSensor::LidarSensor(Vector3 position, Vector3 direction, float fov, float distance)
    : m_position(position), m_direction(direction), m_fov(fov), m_distance(distance),
    m_depthShader(Shader::fromFiles("res/shaders/lidar_depth.vs.glsl", "res/shaders/lidar_depth.fs.glsl")),
    m_visShader(Shader::fromFiles("res/shaders/lidar_vis.vs.glsl", "res/shaders/lidar_vis.fs.glsl")),
    m_vao(0), m_vbo(0), m_ebo(0)
{
    createDepthCubemap();
    buildCubemapViewMatrices();
    generateSphereMesh();
}

LidarSensor::~LidarSensor() 
{
    if (m_depthCubemap != 0) glDeleteTextures(1, &m_depthCubemap);
    if (m_depthCubemapFBO != 0) glDeleteFramebuffers(1, &m_depthCubemapFBO);
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);
}

void LidarSensor::renderDepth(const std::vector<std::pair<Primitive*, Matrix4>> &primitives) 
{
    buildCubemapViewMatrices();

    glBindFramebuffer(GL_FRAMEBUFFER, m_depthCubemapFBO);
    glViewport(0, 0, m_depthMapResolution, m_depthMapResolution);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    //glDisable(GL_CULL_FACE);

    m_depthShader.use();
    m_depthShader.setMat4("projection", m_cubemapProjection.m);
    m_depthShader.setVec3("lightPos", m_position.x, m_position.y, m_position.z);
    m_depthShader.setFloat("farPlane", m_distance);

    for (unsigned int i = 0; i < 6; ++i) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_depthCubemap, 0);
        glClear(GL_DEPTH_BUFFER_BIT);
        // glEnable(GL_DEPTH_TEST);
        // glDepthFunc(GL_LESS);
        // glEnable(GL_CULL_FACE);
        // glCullFace(GL_BACK);
        
        Matrix4 view = m_cubemapView[i];
        view = view.transpose();
        m_depthShader.setMat4("view", view.m);
        for (const auto& [primitive, model] : primitives) {
            Matrix4 modelMatrix = model;
            //modelMatrix = modelMatrix.transpose();
            m_depthShader.setMat4("model", modelMatrix.m);
            primitive->draw(m_depthShader);
        }
    }

    m_depthShader.unuse();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //glEnable(GL_CULL_FACE);
}

void LidarSensor::renderVisualization(const float* cameraView, const float* cameraProjection, int width, int height, unsigned int sceneDepthTexture)
{
    glViewport(0, 0, width, height);

    Matrix4 model = Matrix4::identity();
    model.scaleBy(m_distance, m_distance, m_distance);
    model.translate(m_position.x, m_position.y, m_position.z);
    model = model.transpose();

    // 设置渲染状态
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    m_visShader.use();
    m_visShader.setMat4("model", model.m);
    m_visShader.setMat4("view", cameraView);
    m_visShader.setMat4("projection", cameraProjection);
    m_visShader.setVec3("lidarPosition", m_position.x, m_position.y, m_position.z);
    m_visShader.setVec3("lidarDirection", m_direction.x, m_direction.y, m_direction.z);
    m_visShader.setFloat("fov", m_fov);
    m_visShader.setFloat("maxDistance", m_distance);
    m_visShader.setVec3("color", 0.0f, 0.0f, 0.6f);
    m_visShader.setVec2("screenSize", static_cast<float>(width), static_cast<float>(height));

    // 绑定深度Cubemap
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_depthCubemap);
    m_visShader.setInt("depthCubemap", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sceneDepthTexture);
    m_visShader.setInt("sceneDepthTexture", 1);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // 恢复状态
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

void LidarSensor::generateSphereMesh()
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    int stackCount = 64;
    int sectorCount = 64;
    float radius = 1.0f;
    bool colored = false;
    
    float x, y, z, xy;                              // 顶点位置
    float nx, ny, nz, lengthInv = 1.0f / radius;   // 法线
    float s, t;                                     // 纹理坐标
    
    float sectorStep = 2 * PI / sectorCount;
    float stackStep = PI / stackCount;
    float sectorAngle, stackAngle;
    
    // 生成顶点
    for (int i = 0; i <= stackCount; ++i) {
        stackAngle = PI / 2 - i * stackStep;        // 从 π/2 到 -π/2
        xy = radius * std::cos(stackAngle);         // r * cos(u)
        z = radius * std::sin(stackAngle);          // r * sin(u)
        
        // 为当前 stack 添加 (sectorCount+1) 个顶点
        for (int j = 0; j <= sectorCount; ++j) {
            sectorAngle = j * sectorStep;           // 从 0 到 2π
            
            // 顶点位置 (x, y, z)
            x = xy * std::cos(sectorAngle);         // r * cos(u) * cos(v)
            y = xy * std::sin(sectorAngle);         // r * cos(u) * sin(v)
            
            // 归一化后的法线
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            
            // 纹理坐标
            s = (float)j / sectorCount;
            t = (float)i / stackCount;
            
            // 颜色（基于位置的渐变色或白色）
            float r, g, b;
            if (colored) {
                r = (nx + 1.0f) * 0.5f;  // 将 -1~1 映射到 0~1
                g = (ny + 1.0f) * 0.5f;
                b = (nz + 1.0f) * 0.5f;
            } else {
                r = g = b = 1.0f;
            }
            
            Vertex vertex;
            vertex.position[0] = x;
            vertex.position[1] = y;
            vertex.position[2] = z;
            vertex.color[0] = r;
            vertex.color[1] = g;
            vertex.color[2] = b;
            vertex.normal[0] = nx;
            vertex.normal[1] = ny;
            vertex.normal[2] = nz;
            vertex.texCoord[0] = s;
            vertex.texCoord[1] = t;
            
            vertices.push_back(vertex);
        }
    }
    
    // 生成索引
    // k1--k1+1
    // |  / |
    // | /  |
    // k2--k2+1
    unsigned int k1, k2;
    for (int i = 0; i < stackCount; ++i) {
        k1 = i * (sectorCount + 1);     // 当前 stack 的起始索引
        k2 = k1 + sectorCount + 1;      // 下一个 stack 的起始索引
        
        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            // 每个四边形由两个三角形组成
            // k1 => k2 => k1+1
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            
            // k1+1 => k2 => k2+1
            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // 位置 (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    // 颜色 (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    // 法线 (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // 纹理坐标 (location = 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    m_indexCount = indices.size();
}

void LidarSensor::createDepthCubemap()
{
    glGenTextures(1, &m_depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_depthCubemap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, m_depthMapResolution, m_depthMapResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &m_depthCubemapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_depthCubemapFBO);
    // 先附加第一个面的深度纹理，用于检查FBO完整性
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X, m_depthCubemap, 0);
    // 显式禁用颜色读写
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_CORE_ERROR("Failed to create depth cubemap framebuffer: {}", status);
        return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    float near= 0.001f;
    float far = m_distance;
    float f = PI / 2.0f;
    m_cubemapProjection = Matrix4::perspective(f, 1.0f, near, far);

    m_cubemapProjection = m_cubemapProjection.transpose();
}

void LidarSensor::buildCubemapViewMatrices()
{
    Vector3 targets[6] = {
        Vector3( 1.0f,  0.0f,  0.0f),  // +X
        Vector3(-1.0f,  0.0f,  0.0f),  // -X
        Vector3( 0.0f,  1.0f,  0.0f),  // +Y
        Vector3( 0.0f, -1.0f,  0.0f),  // -Y
        Vector3( 0.0f,  0.0f,  1.0f),  // +Z
        Vector3( 0.0f,  0.0f, -1.0f),  // -Z
    };

    Vector3 ups[6] = {
        Vector3(0.0f, -1.0f,  0.0f),  // +X
        Vector3(0.0f, -1.0f,  0.0f),  // -X
        Vector3(0.0f,  0.0f,  1.0f),  // +Y
        Vector3(0.0f,  0.0f, -1.0f),  // -Y
        Vector3(0.0f, -1.0f,  0.0f),  // +Z
        Vector3(0.0f, -1.0f,  0.0f),  // -Z
    };

    for (unsigned int i = 0; i < 6; ++i) {
        m_cubemapView[i] = Matrix4::lookAt(m_position, m_position + targets[i], ups[i]);
    }
}
RENDERER_NAMESPACE_END