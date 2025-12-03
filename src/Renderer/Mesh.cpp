#include "Mesh.h"
#include <algorithm>
#include <limits>

RENDERER_NAMESPACE_BEGIN

Mesh::Mesh()
    : bboxMin_(std::numeric_limits<float>::max(),
               std::numeric_limits<float>::max(),
               std::numeric_limits<float>::max())
    , bboxMax_(std::numeric_limits<float>::lowest(),
               std::numeric_limits<float>::lowest(),
               std::numeric_limits<float>::lowest())
{
}

Mesh::~Mesh() {
    clear();
}

Mesh::Mesh(Mesh&& other) noexcept
    : vertices_(std::move(other.vertices_))
    , indices_(std::move(other.indices_))
    , materials_(std::move(other.materials_))
    , bboxMin_(other.bboxMin_)
    , bboxMax_(other.bboxMax_)
{
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        vertices_ = std::move(other.vertices_);
        indices_ = std::move(other.indices_);
        materials_ = std::move(other.materials_);
        bboxMin_ = other.bboxMin_;
        bboxMax_ = other.bboxMax_;
    }
    return *this;
}

void Mesh::setVertices(const std::vector<MeshVertex>& vertices) {
    vertices_ = vertices;
    computeBoundingBox();
}

void Mesh::appendVertices(const std::vector<MeshVertex>& vertices) {
    vertices_.insert(vertices_.end(), vertices.begin(), vertices.end());
    computeBoundingBox();
}

void Mesh::setIndices(const std::vector<unsigned int>& indices) {
    indices_ = indices;
}

void Mesh::addMaterial(const Material& material) {
    materials_.push_back(material);
}

void Mesh::computeBoundingBox() {
    if (vertices_.empty()) {
        bboxMin_ = Vector3(0.0f, 0.0f, 0.0f);
        bboxMax_ = Vector3(0.0f, 0.0f, 0.0f);
        return;
    }

    bboxMin_ = Vector3(std::numeric_limits<float>::max(),
                       std::numeric_limits<float>::max(),
                       std::numeric_limits<float>::max());
    bboxMax_ = Vector3(std::numeric_limits<float>::lowest(),
                       std::numeric_limits<float>::lowest(),
                       std::numeric_limits<float>::lowest());

    for (const auto& vertex : vertices_) {
        bboxMin_.x = std::min(bboxMin_.x, vertex.position.x);
        bboxMin_.y = std::min(bboxMin_.y, vertex.position.y);
        bboxMin_.z = std::min(bboxMin_.z, vertex.position.z);

        bboxMax_.x = std::max(bboxMax_.x, vertex.position.x);
        bboxMax_.y = std::max(bboxMax_.y, vertex.position.y);
        bboxMax_.z = std::max(bboxMax_.z, vertex.position.z);
    }
}

void Mesh::clear() {
    vertices_.clear();
    indices_.clear();
    materials_.clear();

    bboxMin_ = Vector3(std::numeric_limits<float>::max(),
                       std::numeric_limits<float>::max(),
                       std::numeric_limits<float>::max());
    bboxMax_ = Vector3(std::numeric_limits<float>::lowest(),
                       std::numeric_limits<float>::lowest(),
                       std::numeric_limits<float>::lowest());
}

RENDERER_NAMESPACE_END
