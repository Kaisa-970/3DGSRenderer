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

    // 设置缓冲区
    setupBuffers(vertices, indices);
    buffersDirty_ = false;
}

void MeshPrimitive::clear() {
    mesh_.reset();
    buffersDirty_ = true;
    // Primitive基类会处理VAO/VBO/EBO的清理
}

RENDERER_NAMESPACE_END
