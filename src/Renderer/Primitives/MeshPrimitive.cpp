#include "MeshPrimitive.h"
#include "Logger/Log.h"
#include <glad/glad.h>

RENDERER_NAMESPACE_BEGIN

MeshPrimitive::MeshPrimitive()
    : mesh_(nullptr)
    , buffersDirty_(true)
{
}

MeshPrimitive::MeshPrimitive(std::shared_ptr<Mesh> mesh)
    : mesh_(mesh)
    , buffersDirty_(true)
{
    if (mesh_) {
        updateBuffers();
    }
}

MeshPrimitive::~MeshPrimitive() {
    clear();
}

MeshPrimitive::MeshPrimitive(MeshPrimitive&& other) noexcept
    : Primitive(std::move(other))
    , mesh_(std::move(other.mesh_))
    , buffersDirty_(other.buffersDirty_)
{
    other.mesh_ = nullptr;
    other.buffersDirty_ = true;
}

MeshPrimitive& MeshPrimitive::operator=(MeshPrimitive&& other) noexcept {
    if (this != &other) {
        Primitive::operator=(std::move(other));
        mesh_ = std::move(other.mesh_);
        buffersDirty_ = other.buffersDirty_;

        other.mesh_ = nullptr;
        other.buffersDirty_ = true;
    }
    return *this;
}

void MeshPrimitive::setMesh(std::shared_ptr<Mesh> mesh) {
    mesh_ = mesh;
    buffersDirty_ = true;

    if (mesh_) {
        updateBuffers();
    }
}

void MeshPrimitive::draw() const {
    if (!mesh_ || mesh_->isEmpty()) return;

    // 确保缓冲区是最新的
    if (buffersDirty_) {
        const_cast<MeshPrimitive*>(this)->updateBuffers();
    }

    // 使用基类的绘制方法
    Primitive::draw();
}

void MeshPrimitive::updateBuffers() {
    if (!mesh_ || mesh_->isEmpty()) return;

    const auto& vertices = mesh_->getVertices();
    //const auto& indices = mesh_->getIndices();
    std::vector<unsigned int> indices = mesh_->getIndices();
    // for (const auto& subMesh : mesh_->getSubMeshes()) {
    //     indices.insert(indices.end(), subMesh.indices.begin(), subMesh.indices.end());
    // }

    // 转换顶点数据为Primitive期望的格式
    std::vector<Vertex> primitiveVertices;
    primitiveVertices.reserve(vertices.size());

    for (const auto& meshVertex : vertices) {
        Vertex v;
        v.position[0] = meshVertex.position.x;
        v.position[1] = meshVertex.position.y;
        v.position[2] = meshVertex.position.z;

        v.normal[0] = meshVertex.normal.x;
        v.normal[1] = meshVertex.normal.y;
        v.normal[2] = meshVertex.normal.z;

        v.texCoord[0] = meshVertex.texCoord.x;
        v.texCoord[1] = meshVertex.texCoord.y;

        // 为网格顶点设置默认颜色（白色）
        // v.color[0] = 1.0f;
        // v.color[1] = 1.0f;
        // v.color[2] = 1.0f;
        v.color[0] = mesh_->getMaterials()[0].diffuseColor.x;
        v.color[1] = mesh_->getMaterials()[0].diffuseColor.y;
        v.color[2] = mesh_->getMaterials()[0].diffuseColor.z;

        primitiveVertices.push_back(v);
    }


    // 设置缓冲区
    setupBuffers(primitiveVertices, indices);
    buffersDirty_ = false;
}

void MeshPrimitive::clear() {
    mesh_.reset();
    buffersDirty_ = true;
    // Primitive基类会处理VAO/VBO/EBO的清理
}

RENDERER_NAMESPACE_END
