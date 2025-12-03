#pragma once

#include "Core/RenderCore.h"
#include "MathUtils/Vector.h"
#include <vector>
#include <string>

RENDERER_NAMESPACE_BEGIN

// 网格顶点数据结构
struct MeshVertex {
    Vector3 position;    // 位置
    Vector3 normal;      // 法线
    Vector2 texCoord;    // UV坐标
    Vector3 tangent;     // 切线（可选）
    Vector3 bitangent;   // 副切线（可选）

    MeshVertex()
        : position(0.0f, 0.0f, 0.0f)
        , normal(0.0f, 0.0f, 1.0f)
        , texCoord(0.0f, 0.0f)
        , tangent(1.0f, 0.0f, 0.0f)
        , bitangent(0.0f, 1.0f, 0.0f) {}
};

// 子网格（材质分组）
struct SubMesh {
    std::string name;
    std::vector<unsigned int> indices;
    unsigned int materialIndex;

    SubMesh() : materialIndex(0) {}
};

// 材质信息
struct Material {
    std::string name;
    Vector3 diffuseColor;
    Vector3 specularColor;
    Vector3 ambientColor;
    float shininess;
    std::string diffuseTexture;
    std::string normalTexture;
    std::string specularTexture;

    Material()
        : diffuseColor(0.8f, 0.8f, 0.8f)
        , specularColor(0.0f, 0.0f, 0.0f)
        , ambientColor(0.2f, 0.2f, 0.2f)
        , shininess(32.0f) {}
};

class RENDERER_API Mesh {
public:
    Mesh();
    ~Mesh();

    // 禁止拷贝
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // 允许移动
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // 数据设置
    void setVertices(const std::vector<MeshVertex>& vertices);
    void appendVertices(const std::vector<MeshVertex>& vertices);  // 追加顶点而不是替换
    void setIndices(const std::vector<unsigned int>& indices);
    void addMaterial(const Material& material);

    // 数据获取
    const std::vector<MeshVertex>& getVertices() const { return vertices_; }
    const std::vector<unsigned int>& getIndices() const { return indices_; }
    const std::vector<Material>& getMaterials() const { return materials_; }

    // 统计信息
    size_t getVertexCount() const { return vertices_.size(); }
    size_t getIndexCount() const { return indices_.size(); }
    size_t getMaterialCount() const { return materials_.size(); }

    // 边界框计算
    void computeBoundingBox();
    const Vector3& getBoundingBoxMin() const { return bboxMin_; }
    const Vector3& getBoundingBoxMax() const { return bboxMax_; }
    Vector3 getCenter() const { return (bboxMin_ + bboxMax_) * 0.5f; }
    Vector3 getSize() const { return bboxMax_ - bboxMin_; }

    // 工具函数
    void clear();
    bool isEmpty() const { return vertices_.empty(); }

private:
    std::vector<MeshVertex> vertices_;
    std::vector<unsigned int> indices_;
    std::vector<Material> materials_;

    // 边界框
    Vector3 bboxMin_;
    Vector3 bboxMax_;
};

RENDERER_NAMESPACE_END
