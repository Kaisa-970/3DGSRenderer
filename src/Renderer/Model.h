#pragma once

#include "Core/RenderCore.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture2D.h"
#include <vector>
#include <string>

RENDERER_NAMESPACE_BEGIN

class RENDERER_API Model {
public:
    Model(std::vector<std::shared_ptr<Mesh>> meshes, std::string directory);
    ~Model();

    void draw(const Shader& shader);
    std::vector<std::shared_ptr<Mesh>>& getMeshes() { return m_meshes; }

    static std::shared_ptr<Model> LoadModelFromFile(const std::string& path);
private:
    std::vector<std::shared_ptr<Mesh>> m_meshes;
    std::string m_directory;
    //std::vector<std::shared_ptr<Texture2D>> m_texturesLoaded;
};

RENDERER_NAMESPACE_END