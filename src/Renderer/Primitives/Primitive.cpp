#include "Primitive.h"
#include <glad/glad.h>
#include <cstddef>

RENDERER_NAMESPACE_BEGIN

Primitive::Primitive()
    : VAO_(0), VBO_(0), EBO_(0), 
      vertexCount_(0), indexCount_(0), hasIndices_(false) {
}

Primitive::~Primitive() {
    cleanup();
}

Primitive::Primitive(Primitive&& other) noexcept
    : VAO_(other.VAO_), VBO_(other.VBO_), EBO_(other.EBO_),
      vertexCount_(other.vertexCount_), indexCount_(other.indexCount_),
      hasIndices_(other.hasIndices_) {
    other.VAO_ = 0;
    other.VBO_ = 0;
    other.EBO_ = 0;
}

Primitive& Primitive::operator=(Primitive&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        VAO_ = other.VAO_;
        VBO_ = other.VBO_;
        EBO_ = other.EBO_;
        vertexCount_ = other.vertexCount_;
        indexCount_ = other.indexCount_;
        hasIndices_ = other.hasIndices_;
        
        other.VAO_ = 0;
        other.VBO_ = 0;
        other.EBO_ = 0;
    }
    return *this;
}

void Primitive::setupBuffers(const std::vector<Vertex>& vertices, 
                              const std::vector<unsigned int>& indices) {
    vertexCount_ = vertices.size();
    indexCount_ = indices.size();
    hasIndices_ = !indices.empty();
    
    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
    
    glBindVertexArray(VAO_);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), 
                 vertices.data(), GL_STATIC_DRAW);
    
    if (hasIndices_) {
        glGenBuffers(1, &EBO_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), 
                     indices.data(), GL_STATIC_DRAW);
    }
    
    // 位置 (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                          (void*)offsetof(Vertex, position));
    
    // 法线 (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                          (void*)offsetof(Vertex, normal));
    
    // 纹理坐标 (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                          (void*)offsetof(Vertex, texCoord));
    
    glBindVertexArray(0);
}

void Primitive::cleanup() {
    if (VAO_ != 0) {
        glDeleteVertexArrays(1, &VAO_);
        VAO_ = 0;
    }
    if (VBO_ != 0) {
        glDeleteBuffers(1, &VBO_);
        VBO_ = 0;
    }
    if (EBO_ != 0) {
        glDeleteBuffers(1, &EBO_);
        EBO_ = 0;
    }
}

void Primitive::draw() const {
    glBindVertexArray(VAO_);
    
    if (hasIndices_) {
        glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, vertexCount_);
    }
    
    glBindVertexArray(0);
}

void Primitive::draw(const Shader& shader) const {
    shader.use();
    draw();
}

RENDERER_NAMESPACE_END