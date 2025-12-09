#include "Material.h"

RENDERER_NAMESPACE_BEGIN

Material::Material()
    : m_name("default")
    , m_diffuseColor(Vector3::ZERO)
    , m_specularColor(Vector3::ZERO)
    , m_ambientColor(Vector3::ZERO)
    , m_shininess(32.0f)
{
}

Material::Material(const std::string& name, const Vector3& diffuseColor, const Vector3& specularColor, const Vector3& ambientColor, float shininess)
    : m_name(name)
    , m_diffuseColor(diffuseColor)
    , m_specularColor(specularColor)
    , m_ambientColor(ambientColor)
    , m_shininess(shininess)
{
}

Material::~Material()
{
}

void Material::setName(const std::string& name)
{
    m_name = name;
}

void Material::setDiffuseColor(const Vector3& diffuseColor)
{
    m_diffuseColor = diffuseColor;
}

void Material::setSpecularColor(const Vector3& specularColor)
{
    m_specularColor = specularColor;
}

void Material::setAmbientColor(const Vector3& ambientColor)
{
    m_ambientColor = ambientColor;
}

void Material::setShininess(float shininess)
{
    m_shininess = shininess;
}

void Material::addDiffuseTexture(const std::shared_ptr<Texture2D>& diffuseTexture)
{
    m_diffuseTextures.push_back(diffuseTexture);
}

void Material::addNormalTexture(const std::shared_ptr<Texture2D>& normalTexture)
{
    m_normalTextures.push_back(normalTexture);
}

void Material::addSpecularTexture(const std::shared_ptr<Texture2D>& specularTexture)
{
    m_specularTextures.push_back(specularTexture);
}

RENDERER_NAMESPACE_END