#pragma once

#include "Core.h"
#include <map>
#include <string>
#include <memory>
#include "Renderer/Texture2D.h"

GSENGINE_NAMESPACE_BEGIN

class GSENGINE_API TextureManager {
public:
    TextureManager();
    ~TextureManager();

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    std::shared_ptr<Renderer::Texture2D> GetTexture2D(const std::string& name);
    std::shared_ptr<Renderer::Texture2D> GetDefaultWhiteTexture();
    std::shared_ptr<Renderer::Texture2D> GetDefaultBlackTexture();
    void AddTexture2D(const std::string& name, const std::shared_ptr<Renderer::Texture2D>& texture, bool overwrite = false);
    void RemoveTexture2D(const std::string& name);

    bool HasTexture2D(const std::string& name) const;
    std::shared_ptr<Renderer::Texture2D> LoadTexture2D(const std::string& path);

private:
    std::map<std::string, std::shared_ptr<Renderer::Texture2D>> m_texture2Ds;
};
GSENGINE_NAMESPACE_END