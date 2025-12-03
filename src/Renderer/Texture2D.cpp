#include "Texture2D.h"
#include <glad/glad.h>
#include "stbimage/stb_image.h"

RENDERER_NAMESPACE_BEGIN

Texture2D::Texture2D()
    : m_textureId(0), m_width(0), m_height(0), m_channels(0) {
        glGenTextures(1, &m_textureId);
    }

Texture2D::~Texture2D() {
    glDeleteTextures(1, &m_textureId);
}

void Texture2D::setData(const void* data, int width, int height, int channels) {
    m_width = width;
    m_height = height;
    m_channels = channels;
    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::bind(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
}

void Texture2D::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

int Texture2D::getWidth() const {
    return m_width;
}

int Texture2D::getHeight() const {
    return m_height;
}

int Texture2D::getChannels() const {
    return m_channels;
}

unsigned int Texture2D::getTextureId() const {
    return m_textureId;
}

std::shared_ptr<Texture2D> Texture2D::createFromFile(const std::string& path) {
    std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>();
    stbi_set_flip_vertically_on_load(1);
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    texture->setData(data, width, height, channels);
    stbi_image_free(data);
    return texture;
}

RENDERER_NAMESPACE_END