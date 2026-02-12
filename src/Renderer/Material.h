#pragma once

#include "Core/RenderCore.h"
#include "MathUtils/Vector.h"
#include "Shader.h"
#include "Texture2D.h"
#include <vector>

RENDERER_NAMESPACE_BEGIN

class RENDERER_API Material
{
public:
    Material();
    Material(const std::string &name, const Vector3 &diffuseColor = VECTOR3_ZERO,
             const Vector3 &specularColor = VECTOR3_ONE, const Vector3 &ambientColor = VECTOR3_ZERO,
             float shininess = 32.0f);
    ~Material();

    void setName(const std::string &name);
    void setDiffuseColor(const Vector3 &diffuseColor);
    void setSpecularColor(const Vector3 &specularColor);
    void setAmbientColor(const Vector3 &ambientColor);
    void setShininess(float shininess);

    void addDiffuseTexture(const std::shared_ptr<Texture2D> &diffuseTexture);
    void addNormalTexture(const std::shared_ptr<Texture2D> &normalTexture);
    void addSpecularTexture(const std::shared_ptr<Texture2D> &specularTexture);

    const std::string &getName() const
    {
        return m_name;
    }
    const Vector3 &getDiffuseColor() const
    {
        return m_diffuseColor;
    }
    const Vector3 &getSpecularColor() const
    {
        return m_specularColor;
    }
    const Vector3 &getAmbientColor() const
    {
        return m_ambientColor;
    }
    float getShininess() const
    {
        return m_shininess;
    }
    const std::vector<std::shared_ptr<Texture2D>> &getDiffuseTextures() const
    {
        return m_diffuseTextures;
    }
    const std::vector<std::shared_ptr<Texture2D>> &getNormalTextures() const
    {
        return m_normalTextures;
    }
    const std::vector<std::shared_ptr<Texture2D>> &getSpecularTextures() const
    {
        return m_specularTextures;
    }

    void UpdateShaderParams(const std::shared_ptr<Shader> &shader) const;

private:
    std::string m_name;
    Vector3 m_diffuseColor;
    Vector3 m_specularColor;
    Vector3 m_ambientColor;
    float m_shininess;
    std::vector<std::shared_ptr<Texture2D>> m_diffuseTextures;
    std::vector<std::shared_ptr<Texture2D>> m_normalTextures;
    std::vector<std::shared_ptr<Texture2D>> m_specularTextures;
};

RENDERER_NAMESPACE_END
