#pragma once

#include "Renderer/Primitives/Primitive.h"
#include "Renderer/Mesh.h"
#include <memory>

RENDERER_NAMESPACE_BEGIN

class RENDERER_API MeshPrimitive : public Primitive {
public:
    MeshPrimitive();
    explicit MeshPrimitive(std::shared_ptr<Mesh> mesh);
    virtual ~MeshPrimitive();

    // 禁止拷贝
    MeshPrimitive(const MeshPrimitive&) = delete;
    MeshPrimitive& operator=(const MeshPrimitive&) = delete;

    // 允许移动
    MeshPrimitive(MeshPrimitive&& other) noexcept;
    MeshPrimitive& operator=(MeshPrimitive&& other) noexcept;

    // 设置网格数据
    void setMesh(std::shared_ptr<Mesh> mesh);
    const std::shared_ptr<Mesh>& getMesh() const { return mesh_; }

    // 渲染（重写基类方法）
    virtual void draw() const override;

    // 获取网格信息
    size_t getVertexCount() const { return mesh_ ? mesh_->getVertexCount() : 0; }
    size_t getIndexCount() const { return mesh_ ? mesh_->getIndexCount() : 0; }

    // 边界框
    const Vector3& getBoundingBoxMin() const { return mesh_ ? mesh_->getBoundingBoxMin() : Vector3(0.0f, 0.0f, 0.0f); }
    const Vector3& getBoundingBoxMax() const { return mesh_ ? mesh_->getBoundingBoxMax() : Vector3(0.0f, 0.0f, 0.0f); }
    Vector3 getCenter() const { return mesh_ ? mesh_->getCenter() : Vector3(0.0f, 0.0f, 0.0f); }
    Vector3 getSize() const { return mesh_ ? mesh_->getSize() : Vector3(0.0f, 0.0f, 0.0f); }

    // 工具函数
    void clear();
    bool isEmpty() const { return !mesh_ || mesh_->isEmpty(); }

private:
    // 更新缓冲区数据
    void updateBuffers();

    std::shared_ptr<Mesh> mesh_;
    bool buffersDirty_;  // 标记缓冲区是否需要更新
};

RENDERER_NAMESPACE_END
