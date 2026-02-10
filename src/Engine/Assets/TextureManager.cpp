#include "TextureManager.h"
#include "Logger/Log.h"

GSENGINE_NAMESPACE_BEGIN

const std::string DEFAULT_WHITE_TEXTURE_NAME = "3DGSRendererDefaultWhiteTexture";
const std::string DEFAULT_BLACK_TEXTURE_NAME = "3DGSRendererDefaultBlackTexture";

TextureManager::TextureManager()
{
    std::shared_ptr<Renderer::Texture2D> whiteTexture = std::make_shared<Renderer::Texture2D>();
    unsigned char data[4] = {255, 255, 255, 255};
    whiteTexture->setData(data, 1, 1, 4);
    AddTexture2D(DEFAULT_WHITE_TEXTURE_NAME, whiteTexture, true);

    std::shared_ptr<Renderer::Texture2D> blackTexture = std::make_shared<Renderer::Texture2D>();
    unsigned char data2[4] = {0, 0, 0, 255};
    blackTexture->setData(data2, 1, 1, 4);
    AddTexture2D(DEFAULT_BLACK_TEXTURE_NAME, blackTexture, true);
}

TextureManager::~TextureManager()
{
    m_texture2Ds.clear();
}

std::shared_ptr<Renderer::Texture2D> TextureManager::GetTexture2D(const std::string &name)
{
    auto it = m_texture2Ds.find(name);
    if (it != m_texture2Ds.end())
    {
        return it->second;
    }
    LOG_CORE_ERROR("Texture2D not found: {}", name);
    return nullptr;
}

std::shared_ptr<Renderer::Texture2D> TextureManager::GetDefaultWhiteTexture()
{
    return GetTexture2D(DEFAULT_WHITE_TEXTURE_NAME);
}

std::shared_ptr<Renderer::Texture2D> TextureManager::GetDefaultBlackTexture()
{
    return GetTexture2D(DEFAULT_BLACK_TEXTURE_NAME);
}

void TextureManager::AddTexture2D(const std::string &name, const std::shared_ptr<Renderer::Texture2D> &texture,
                                  bool overwrite)
{
    auto it = m_texture2Ds.find(name);
    if (it != m_texture2Ds.end())
    {
        if (overwrite)
        {
            m_texture2Ds[name] = texture;
            return;
        }
        LOG_CORE_ERROR("Texture2D already exists: {}", name);
        return;
    }

    m_texture2Ds[name] = texture;
}

void TextureManager::RemoveTexture2D(const std::string &name)
{
    auto it = m_texture2Ds.find(name);
    if (it != m_texture2Ds.end())
    {
        m_texture2Ds.erase(it);
        return;
    }
}

bool TextureManager::HasTexture2D(const std::string &name) const
{
    return m_texture2Ds.find(name) != m_texture2Ds.end();
}

std::shared_ptr<Renderer::Texture2D> TextureManager::LoadTexture2D(const std::string &path)
{
    auto it = m_texture2Ds.find(path);
    if (it != m_texture2Ds.end())
    {
        LOG_CORE_WARN("Texture2D already loaded: {}", path);
        return it->second;
    }
    LOG_CORE_INFO("Loading texture2D from path: {}", path);
    std::shared_ptr<Renderer::Texture2D> texture = Renderer::Texture2D::createFromFile(path);
    if (texture)
    {
        AddTexture2D(path, texture, true);
        return texture;
    }
    LOG_CORE_ERROR("Failed to load texture2D from path: {}", path);
    return nullptr;
}

GSENGINE_NAMESPACE_END
