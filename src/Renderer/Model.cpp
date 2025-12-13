#include "Model.h"

RENDERER_NAMESPACE_BEGIN

Model::Model(std::vector<SubMesh> subMeshes, std::string directory)
    : m_subMeshes(subMeshes), m_directory(directory)
{

}

Model::~Model()
{
    m_subMeshes.clear();
}

void Model::draw(const std::shared_ptr<Shader>& shader)
{
    shader->use();
    for (auto& subMesh : m_subMeshes) 
    {
        if (subMesh.material) 
        {
            shader->setFloat("u_shininess", subMesh.material->getShininess());
            shader->setVec3("u_diffuseColor", subMesh.material->getDiffuseColor().x, subMesh.material->getDiffuseColor().y, subMesh.material->getDiffuseColor().z);
            shader->setVec3("u_specularColor", subMesh.material->getSpecularColor().x, subMesh.material->getSpecularColor().y, subMesh.material->getSpecularColor().z);

            if (!subMesh.material->getDiffuseTextures().empty()) {
                subMesh.material->getDiffuseTextures()[0]->bind(0);
                shader->setInt("u_diffuseTexture", 0);
            }
            if (!subMesh.material->getSpecularTextures().empty()) {
                subMesh.material->getSpecularTextures()[0]->bind(1);
                shader->setInt("u_specularTexture", 1);
            }
            if (!subMesh.material->getNormalTextures().empty()) {
                subMesh.material->getNormalTextures()[0]->bind(2);
                shader->setInt("u_normalTexture", 2);
            }
        }

        if (subMesh.mesh) {
            subMesh.mesh->draw(shader);
        }
    }
}

RENDERER_NAMESPACE_END