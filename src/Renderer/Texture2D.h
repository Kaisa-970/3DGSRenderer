#pragma once

#include "Core/RenderCore.h"
#include <string>
#include <memory>

RENDERER_NAMESPACE_BEGIN

class RENDERER_API Texture2D
{
public:
    Texture2D();
    ~Texture2D();

    void setData(const void *data, int width, int height, int channels);
    void getData(void *data) const;
    void bind(int slot = 0) const;
    void unbind() const;
    int getWidth() const;
    int getHeight() const;
    int getChannels() const;
    unsigned int getTextureId() const;

    static std::shared_ptr<Texture2D> createFromFile(const std::string &path);
    static bool SaveTexture2DPNG(const std::string &path, const std::shared_ptr<Texture2D> &texture);
    static bool SaveTexture2DPNG(const std::string &path, unsigned int textureId, int width, int height,
                                 int channels = 4);

private:
    unsigned int m_textureId;
    int m_width;
    int m_height;
    int m_channels;
};

RENDERER_NAMESPACE_END
