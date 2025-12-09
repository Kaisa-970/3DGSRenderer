#pragma once

#include "Core/RenderCore.h"
#include <map>
#include <string>
#include <memory>
#include "Texture2D.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API TextureManager {
public:
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    static TextureManager* GetInstance() { 
        static TextureManager* instance = new TextureManager();
        return instance; 
    }

    std::shared_ptr<Texture2D> GetTexture2D(const std::string& name);
    std::shared_ptr<Texture2D> GetDefaultWhiteTexture();
    std::shared_ptr<Texture2D> GetDefaultBlackTexture();
    void AddTexture2D(const std::string& name, const std::shared_ptr<Texture2D>& texture, bool overwrite = false);
    void RemoveTexture2D(const std::string& name);

    bool HasTexture2D(const std::string& name) const;
    std::shared_ptr<Texture2D> LoadTexture2D(const std::string& path);
private:
    TextureManager();
    ~TextureManager();

private:
    std::map<std::string, std::shared_ptr<Texture2D>> m_texture2Ds;
};
RENDERER_NAMESPACE_END