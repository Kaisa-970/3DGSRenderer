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
    , m_materials(std::move(other.m_materials))
    , bboxMin_(other.bboxMin_)
    , bboxMax_(other.bboxMax_)
{
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        vertices_ = std::move(other.vertices_);
        indices_ = std::move(other.indices_);
        m_materials = std::move(other.m_materials);
        bboxMin_ = other.bboxMin_;
        bboxMax_ = other.bboxMax_;
    }
    return *this;
}

void Mesh::setVertices(const std::vector<Vertex>& vertices) {
    vertices_ = vertices;
    computeBoundingBox();
}

void Mesh::setIndices(const std::vector<unsigned int>& indices) {
    indices_ = indices;
    computeBoundingBox();
}

void Mesh::addMaterial(const std::shared_ptr<Material>& material) {
    m_materials.push_back(material);
}

void Mesh::Setup() {
    setupBuffers(vertices_, indices_);
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
    m_materials.clear();

    bboxMin_ = Vector3(std::numeric_limits<float>::max(),
                       std::numeric_limits<float>::max(),
                       std::numeric_limits<float>::max());
    bboxMax_ = Vector3(std::numeric_limits<float>::lowest(),
                       std::numeric_limits<float>::lowest(),
                       std::numeric_limits<float>::lowest());
}

void Mesh::draw(const Shader& shader) const {
    if (m_materials.empty()) {
        return;
    }
    std::shared_ptr<Material> material = m_materials[0];
    if (material) 
    {
        if (material->getDiffuseTextures().size() > 0) 
        {
            auto diffuseTexture = material->getDiffuseTextures()[0];
            diffuseTexture->bind(0);
            shader.setInt("u_diffuseTexture", 0);
        }
        if (material->getNormalTextures().size() > 0) {
            auto normalTexture = material->getNormalTextures()[0];
            normalTexture->bind(1);
            shader.setInt("u_normalTexture", 1);
        }
        if (material->getSpecularTextures().size() > 0) {
            auto specularTexture = material->getSpecularTextures()[0];
            specularTexture->bind(2);
            shader.setInt("u_specularTexture", 2);
        }

        shader.setFloat("u_shininess", material->getShininess());
        shader.setVec3("u_diffuseColor", material->getDiffuseColor().x, material->getDiffuseColor().y, material->getDiffuseColor().z);
        shader.setVec3("u_specularColor", material->getSpecularColor().x, material->getSpecularColor().y, material->getSpecularColor().z);
    }
    Primitive::draw(shader);
}

RENDERER_NAMESPACE_END
