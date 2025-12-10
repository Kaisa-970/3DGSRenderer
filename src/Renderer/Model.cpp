#include "Model.h"
#include "AssimpModelLoader.h"
#include "Logger/Log.h"

RENDERER_NAMESPACE_BEGIN

Model::Model(std::vector<std::shared_ptr<Mesh>> meshes, std::string directory)
    : m_meshes(meshes), m_directory(directory)
{

}

Model::~Model()
{
    m_meshes.clear();
}

void Model::draw(const Shader& shader)
{
    for (auto& mesh : m_meshes) 
    {
        mesh->draw(shader);
    }
}

std::shared_ptr<Model> Model::LoadModelFromFile(const std::string& path)
{
    AssimpModelLoader loader;
    return loader.loadModel(path);
}
RENDERER_NAMESPACE_END