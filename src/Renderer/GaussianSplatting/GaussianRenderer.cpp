#include "GaussianSplatting/GaussianRenderer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <glad/glad.h>

RENDERER_NAMESPACE_BEGIN

GaussianRenderer::GaussianRenderer()
    : m_shader(Renderer::Shader::fromFiles("res/shaders/point.vs.glsl", "res/shaders/point.fs.glsl")) {
}

GaussianRenderer::~GaussianRenderer() {
}

void GaussianRenderer::loadModel(const std::string& path) 
{
    std::ifstream ifs(path, std::ios_base::binary);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open model file: " << path << std::endl;
        return;
    }

    // std::string line;
    // while (std::getline(ifs, line)) {
    //     std::istringstream iss(line);

    std::string buffer;
    std::getline(ifs, buffer);
    std::getline(ifs, buffer);
    std::getline(ifs, buffer);

    std::stringstream ss(buffer);
    std::string dummy;
    ss >> dummy;
    ss >> dummy;
    int count;
    ss >> count;
    std::cout << "Load model: " << path << " with " << count << " points" << std::endl;

    while (buffer != "end_header") {
        std::getline(ifs, buffer);
    }

    m_vertexCount = count;
    m_points.resize(m_vertexCount);
    ifs.read(reinterpret_cast<char*>(m_points.data()), m_vertexCount * sizeof(NormalPoint));

    float scale = 1.0f / 255.0f;
    for (int i = 0; i < m_vertexCount; i++) 
    {
        m_points[i].position[1] = -m_points[i].position[1];
        //m_points[i].position[0] = -m_points[i].position[0];
        //m_points[i].position[2] = -m_points[i].position[2];
    }
    ifs.close();

    setupBuffers();
}

void GaussianRenderer::drawPoints(const Renderer::Matrix4& model, const Renderer::Matrix4& view, const Renderer::Matrix4& projection)
{
    m_shader.use();
    m_shader.setMat4("model", model.data());
    m_shader.setMat4("view", view.data());
    m_shader.setMat4("projection", projection.data());
    glPointSize(2.0f);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_POINTS, 0, m_vertexCount);
    glBindVertexArray(0);
}

void GaussianRenderer::setupBuffers()
{
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    std::cout << "Size of NormalPoint: " << sizeof(NormalPoint) << std::endl;
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertexCount * sizeof(NormalPoint), m_points.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(NormalPoint), (void*)offsetof(NormalPoint, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(NormalPoint), (void*)offsetof(NormalPoint, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(NormalPoint), (void*)offsetof(NormalPoint, color));

    glBindVertexArray(0);
}

RENDERER_NAMESPACE_END