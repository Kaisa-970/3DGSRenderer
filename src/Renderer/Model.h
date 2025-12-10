#pragma once

#include "Core/RenderCore.h"
#include "Shader.h"
#include "Mesh.h"
#include "Material.h"
#include <vector>
#include <string>

RENDERER_NAMESPACE_BEGIN

struct SubMesh
{
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
};

class RENDERER_API Model {
public:
    Model(std::vector<SubMesh> subMeshes, std::string directory);
    ~Model();

    void draw(const Shader& shader);
    std::vector<SubMesh>& getSubMeshes() { return m_subMeshes; }

    static std::shared_ptr<Model> LoadModelFromFile(const std::string& path);
private:
    std::vector<SubMesh> m_subMeshes;
    std::string m_directory;
    //std::vector<std::shared_ptr<Texture2D>> m_texturesLoaded;
};

RENDERER_NAMESPACE_END