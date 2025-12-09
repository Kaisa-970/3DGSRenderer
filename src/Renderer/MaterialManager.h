#pragma once

#include "Core/RenderCore.h"
#include <map>
#include <string>
#include <memory>
#include "Material.h"
RENDERER_NAMESPACE_BEGIN

class RENDERER_API MaterialManager {
public:
    MaterialManager(const MaterialManager&) = delete;
    MaterialManager& operator=(const MaterialManager&) = delete;
    static MaterialManager* GetInstance() { 
        static MaterialManager* instance = new MaterialManager();
        return instance; 
    }
    std::shared_ptr<Material> GetMaterial(const std::string& name);
    std::shared_ptr<Material> GetDefaultMaterial();
    void AddMaterial(const std::string& name, const std::shared_ptr<Material>& material, bool overwrite = false);
    void RemoveMaterial(const std::string& name);
    bool HasMaterial(const std::string& name) const;
private:
    MaterialManager();
    ~MaterialManager();
private:
    std::map<std::string, std::shared_ptr<Material>> m_materials;
};

RENDERER_NAMESPACE_END