#pragma once

#include "Core.h"
#include <memory>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Renderer/MathUtils/Matrix.h"
#include "Renderer/Model.h"

GSENGINE_NAMESPACE_BEGIN

class GSENGINE_API AssimpModelLoader {
public:
    AssimpModelLoader();
    ~AssimpModelLoader();

    // 加载模型文件（支持多种格式）
    //std::vector<std::shared_ptr<Mesh>> loadModel(const std::string& filename);
    std::shared_ptr< Renderer::Model> loadModel(const std::string& filename);

    // 获取支持的文件格式列表
    static std::vector<std::string> getSupportedFormats();

    // 检查文件格式是否支持
    static bool isFormatSupported(const std::string& extension);

    // 获取加载统计信息
    size_t getLoadedVertices() const { return totalVertices_; }
    size_t getLoadedFaces() const { return totalFaces_; }
    size_t getLoadedMeshes() const { return totalMeshes_; }

private:
    // 处理Assimp场景
    std::vector<Renderer::SubMesh> processScene(const aiScene* scene, const std::string& filename);

    // 处理单个网格
    void processMesh(aiMesh* mesh, const aiScene* scene, Renderer::Mesh& outMesh, const Renderer::Matrix4& transform);

    // 处理材质
    void processMaterial(aiMaterial* material, const aiScene* scene, const std::string& directory, std::shared_ptr<Renderer::Material>& outMaterial);

    // 处理节点（递归）
    void processNode(aiNode* node, const aiScene* scene, std::vector<Renderer::SubMesh>& subMeshes, const Renderer::Matrix4& parentTransform, const std::vector<std::shared_ptr<Renderer::Material>>& materials);

    // 转换Assimp向量到我们的向量类型
    Renderer::Vector3 assimpToVector3(const aiVector3D& v);
    Renderer::Vector2 assimpToVector2(const aiVector2D& v);
    Renderer::Vector3 assimpToColor3(const aiColor3D& c);
    //Vector4 assimpToColor4(const aiColor4D& c);

    // 转换Assimp矩阵到我们的矩阵类型
    Renderer::Matrix4 assimpToMatrix4(const aiMatrix4x4& m);

    // // 应用变换到顶点列表
    // void applyTransformToVertices(std::vector<Vertex>& vertices, const Matrix4& transform);

    // 加载统计
    size_t totalVertices_;
    size_t totalFaces_;
    size_t totalMeshes_;
};

GSENGINE_NAMESPACE_END
