#include "MaterialManager.h"
#include "Logger/Log.h"
#include "TextureManager.h"

GSENGINE_NAMESPACE_BEGIN
const std::string DEFAULT_MATERIAL_NAME = "3DGSRendererDefaultMaterial";

MaterialManager::MaterialManager(TextureManager& textureManager)
    : m_textureManager(textureManager)
{
    std::shared_ptr<Renderer::Material> defaultMaterial = std::make_shared<Renderer::Material>(DEFAULT_MATERIAL_NAME);
    defaultMaterial->addDiffuseTexture(m_textureManager.GetDefaultWhiteTexture());
    defaultMaterial->addSpecularTexture(m_textureManager.GetDefaultWhiteTexture());
    defaultMaterial->addNormalTexture(m_textureManager.GetDefaultBlackTexture());
    defaultMaterial->setDiffuseColor(Renderer::Vector3::ONE);
    defaultMaterial->setSpecularColor(Renderer::Vector3::ONE);
    defaultMaterial->setShininess(32.0f);
    AddMaterial(DEFAULT_MATERIAL_NAME, defaultMaterial, true);
}

MaterialManager::~MaterialManager()
{
    m_materials.clear();
}

std::shared_ptr<Renderer::Material> MaterialManager::GetMaterial(const std::string& name)
{
    auto it = m_materials.find(name);
    if (it != m_materials.end())
    {
        return it->second;
    }
    LOG_CORE_ERROR("Material not found: {}", name);
    return nullptr;
}

std::shared_ptr<Renderer::Material> MaterialManager::GetDefaultMaterial()
{
    return GetMaterial(DEFAULT_MATERIAL_NAME);
}

void MaterialManager::AddMaterial(const std::string& name, const std::shared_ptr<Renderer::Material>& material, bool overwrite)
{
    auto it = m_materials.find(name);
    if (it != m_materials.end())
    {
        if (overwrite)
        {
            m_materials[name] = material;
            return;
        }
        LOG_CORE_ERROR("Material already exists: {}", name);
        return;
    }
    m_materials[name] = material;
}

void MaterialManager::RemoveMaterial(const std::string& name)
{
    auto it = m_materials.find(name);
    if (it != m_materials.end())
    {
        m_materials.erase(it);
    }
}

bool MaterialManager::HasMaterial(const std::string& name) const
{
    return m_materials.find(name) != m_materials.end();
}

GSENGINE_NAMESPACE_END