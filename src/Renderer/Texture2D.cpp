#include "Texture2D.h"
#include <glad/glad.h>
#include "stbimage/stb_image.h"
#include "stbimage/stb_image_write.h"
#include "Logger/Log.h"

RENDERER_NAMESPACE_BEGIN

Texture2D::Texture2D() : m_textureId(0), m_width(0), m_height(0), m_channels(0)
{
    glGenTextures(1, &m_textureId);
}

Texture2D::~Texture2D()
{
    glDeleteTextures(1, &m_textureId);
}

void Texture2D::setData(const void *data, int width, int height, int channels)
{
    m_width = width;
    m_height = height;
    m_channels = channels;
    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::getData(void *data) const
{
    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glGetTexImage(GL_TEXTURE_2D, 0, m_channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::bind(int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
}

void Texture2D::unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

int Texture2D::getWidth() const
{
    return m_width;
}

int Texture2D::getHeight() const
{
    return m_height;
}

int Texture2D::getChannels() const
{
    return m_channels;
}

unsigned int Texture2D::getTextureId() const
{
    return m_textureId;
}

std::shared_ptr<Texture2D> Texture2D::createFromFile(const std::string &path)
{
    std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>();
    stbi_set_flip_vertically_on_load(1);
    int width, height, channels;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data)
    {
        LOG_CORE_ERROR("Failed to load texture2D from path: {}", path);
        return nullptr;
    }
    texture->setData(data, width, height, channels);
    stbi_image_free(data);
    return texture;
}

bool Texture2D::SaveTexture2DPNG(const std::string &path, const std::shared_ptr<Texture2D> &texture)
{
    int width = texture->getWidth();
    int height = texture->getHeight();
    int channels = texture->getChannels();
    unsigned char *data = new unsigned char[width * height * channels];
    texture->getData(data);
    stbi_flip_vertically_on_write(1);
    int result = stbi_write_png(path.c_str(), width, height, channels, data, width * channels);
    delete[] data;
    return result != 0;
}

bool Texture2D::SaveTexture2DPNG(const std::string &path, unsigned int textureId, int width, int height, int channels)
{
    glBindTexture(GL_TEXTURE_2D, textureId);
    unsigned char *data = new unsigned char[width * height * channels];
    glGetTexImage(GL_TEXTURE_2D, 0, channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
    stbi_flip_vertically_on_write(1);
    int result = stbi_write_png(path.c_str(), width, height, channels, data, width * channels);
    delete[] data;
    glBindTexture(GL_TEXTURE_2D, 0);
    return result != 0;
}
RENDERER_NAMESPACE_END
