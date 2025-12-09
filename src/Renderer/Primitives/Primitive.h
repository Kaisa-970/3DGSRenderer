#pragma once

#include "Core/RenderCore.h"
#include "Shader.h"
#include <vector>
#include "MathUtils/Vector.h"

RENDERER_NAMESPACE_BEGIN

struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 texCoord;
};

class RENDERER_API Primitive {
public:
    Primitive();
    virtual ~Primitive();

    // 禁止拷贝
    Primitive(const Primitive&) = delete;
    Primitive& operator=(const Primitive&) = delete;

    // 允许移动
    Primitive(Primitive&& other) noexcept;
    Primitive& operator=(Primitive&& other) noexcept;

    // 绘制
    virtual void draw() const;
    virtual void draw(const Shader& shader) const;
    
    // 获取信息
    unsigned int getVertexCount() const { return vertexCount_; }
    unsigned int getIndexCount() const { return indexCount_; }

protected:
    unsigned int VAO_;
    unsigned int VBO_;
    unsigned int EBO_;
    unsigned int vertexCount_;
    unsigned int indexCount_;
    bool hasIndices_;

    // 由子类调用来初始化几何体
    void setupBuffers(const std::vector<Vertex>& vertices, 
                      const std::vector<unsigned int>& indices = {});
    void cleanup();
};

RENDERER_NAMESPACE_END