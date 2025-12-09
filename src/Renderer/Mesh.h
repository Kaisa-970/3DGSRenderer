#pragma once

#include "Core/RenderCore.h"
#include "MathUtils/Vector.h"
#include <vector>
#include <memory>
#include "Material.h"
#include "Primitives/Primitive.h"
RENDERER_NAMESPACE_BEGIN

// // 网格顶点数据结构
// struct MeshVertex {
//     Vector3 position;    // 位置
//     Vector3 normal;      // 法线
//     Vector2 texCoord;    // UV坐标

//     MeshVertex()
//         : position(0.0f, 0.0f, 0.0f)
//         , normal(0.0f, 0.0f, 1.0f)
//         , texCoord(0.0f, 0.0f)
//     {
//     }
// };

class RENDERER_API Mesh : public Primitive {
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
    void setVertices(const std::vector<Vertex>& vertices);
    void setIndices(const std::vector<unsigned int>& indices);
    void addMaterial(const std::shared_ptr<Material>& material);

    void Setup();

    // 数据获取
    const std::vector<Vertex>& getVertices() const { return vertices_; }
    const std::vector<unsigned int>& getIndices() const { return indices_; }
    const std::vector<std::shared_ptr<Material>>& getMaterials() const { return m_materials; }

    // 统计信息
    size_t getVertexCount() const { return vertices_.size(); }
    size_t getIndexCount() const { return indices_.size(); }
    size_t getMaterialCount() const { return m_materials.size(); }

    // 边界框计算
    void computeBoundingBox();
    const Vector3& getBoundingBoxMin() const { return bboxMin_; }
    const Vector3& getBoundingBoxMax() const { return bboxMax_; }
    Vector3 getCenter() const { return (bboxMin_ + bboxMax_) * 0.5f; }
    Vector3 getSize() const { return bboxMax_ - bboxMin_; }

    // 工具函数
    void clear();
    bool isEmpty() const { return vertices_.empty(); }

    virtual void draw(const Shader& shader) const override;
private:
    std::vector<Vertex> vertices_;
    std::vector<unsigned int> indices_;
    std::vector<std::shared_ptr<Material>> m_materials;

    // 边界框
    Vector3 bboxMin_;
    Vector3 bboxMax_;
};

RENDERER_NAMESPACE_END
