#include "Material.h"

RENDERER_NAMESPACE_BEGIN

Material::Material()
    : m_name("default"), m_diffuseColor(VECTOR3_ZERO), m_specularColor(VECTOR3_ZERO), m_ambientColor(VECTOR3_ZERO),
      m_shininess(32.0f)
{
}

Material::Material(const std::string &name, const Vector3 &diffuseColor, const Vector3 &specularColor,
                   const Vector3 &ambientColor, float shininess)
    : m_name(name), m_diffuseColor(diffuseColor), m_specularColor(specularColor), m_ambientColor(ambientColor),
      m_shininess(shininess)
{
}

Material::~Material()
{
}

void Material::setName(const std::string &name)
{
    m_name = name;
}

void Material::setDiffuseColor(const Vector3 &diffuseColor)
{
    m_diffuseColor = diffuseColor;
}

void Material::setSpecularColor(const Vector3 &specularColor)
{
    m_specularColor = specularColor;
}

void Material::setAmbientColor(const Vector3 &ambientColor)
{
    m_ambientColor = ambientColor;
}

void Material::setShininess(float shininess)
{
    m_shininess = shininess;
}

void Material::addDiffuseTexture(const std::shared_ptr<Texture2D> &diffuseTexture)
{
    m_diffuseTextures.push_back(diffuseTexture);
}

void Material::addNormalTexture(const std::shared_ptr<Texture2D> &normalTexture)
{
    m_normalTextures.push_back(normalTexture);
}

void Material::addSpecularTexture(const std::shared_ptr<Texture2D> &specularTexture)
{
    m_specularTextures.push_back(specularTexture);
}

void Material::UpdateShaderParams(const std::shared_ptr<Shader> &shader) const
{
    shader->setVec3("u_diffuseColor", m_diffuseColor.x, m_diffuseColor.y, m_diffuseColor.z);
    shader->setVec3("u_specularColor", m_specularColor.x, m_specularColor.y, m_specularColor.z);
    shader->setFloat("shininess", m_shininess);
    if (m_diffuseTextures.size() > 0)
    {
        m_diffuseTextures[0]->bind(0);
        shader->setInt("u_diffuseTexture", 0);
    }
    if (m_specularTextures.size() > 0)
    {
        m_specularTextures[0]->bind(1);
        shader->setInt("u_specularTexture", 1);
    }
    if (m_normalTextures.size() > 0)
    {
        m_normalTextures[0]->bind(2);
        shader->setInt("u_normalTexture", 2);
    }
}

RENDERER_NAMESPACE_END
