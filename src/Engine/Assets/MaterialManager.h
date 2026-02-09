#pragma once

#include "Core.h"
#include <map>
#include <string>
#include <memory>
#include "Renderer/Material.h"

GSENGINE_NAMESPACE_BEGIN

class TextureManager;  // 前向声明

class GSENGINE_API MaterialManager {
public:
    // 通过依赖注入接收 TextureManager
    explicit MaterialManager(TextureManager& textureManager);
    ~MaterialManager();

    MaterialManager(const MaterialManager&) = delete;
    MaterialManager& operator=(const MaterialManager&) = delete;

    std::shared_ptr<Renderer::Material> GetMaterial(const std::string& name);
    std::shared_ptr<Renderer::Material> GetDefaultMaterial();
    void AddMaterial(const std::string& name, const std::shared_ptr<Renderer::Material>& material, bool overwrite = false);
    void RemoveMaterial(const std::string& name);
    bool HasMaterial(const std::string& name) const;

private:
    TextureManager& m_textureManager;
    std::map<std::string, std::shared_ptr<Renderer::Material>> m_materials;
};

GSENGINE_NAMESPACE_END