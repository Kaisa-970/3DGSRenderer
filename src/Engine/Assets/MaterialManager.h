#pragma once

#include "Core.h"
#include <map>
#include <string>
#include <memory>
#include "Renderer/Material.h"
GSENGINE_NAMESPACE_BEGIN

class GSENGINE_API MaterialManager {
public:
    MaterialManager(const MaterialManager&) = delete;
    MaterialManager& operator=(const MaterialManager&) = delete;
    static MaterialManager* GetInstance() { 
        static MaterialManager* instance = new MaterialManager();
        return instance; 
    }
    std::shared_ptr<Renderer::Material> GetMaterial(const std::string& name);
    std::shared_ptr<Renderer::Material> GetDefaultMaterial();
    void AddMaterial(const std::string& name, const std::shared_ptr<Renderer::Material>& material, bool overwrite = false);
    void RemoveMaterial(const std::string& name);
    bool HasMaterial(const std::string& name) const;
private:
    MaterialManager();
    ~MaterialManager();
private:
    std::map<std::string, std::shared_ptr<Renderer::Material>> m_materials;
};

GSENGINE_NAMESPACE_END