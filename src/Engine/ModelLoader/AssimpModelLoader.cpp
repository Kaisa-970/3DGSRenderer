#include "AssimpModelLoader.h"
#include "Logger/Log.h"
#include <algorithm>
#include <filesystem>
#include "Assets/MaterialManager.h"
#include "Assets/TextureManager.h"

using namespace Renderer;
GSENGINE_NAMESPACE_BEGIN

AssimpModelLoader::AssimpModelLoader(TextureManager &textureManager, MaterialManager &materialManager)
    : textureManager_(textureManager), materialManager_(materialManager), totalVertices_(0), totalFaces_(0),
      totalMeshes_(0)
{
}

AssimpModelLoader::~AssimpModelLoader()
{
}

std::shared_ptr<Model> AssimpModelLoader::loadModel(const std::string &filename)
{
    LOG_INFO("Loading model with Assimp: {}", filename);

    // 重置统计信息
    totalVertices_ = 0;
    totalFaces_ = 0;
    totalMeshes_ = 0;

    // 检查文件是否存在
    if (!std::filesystem::exists(filename))
    {
        LOG_ERROR("Model file does not exist: {}", filename);
        return std::shared_ptr<Model>();
    }

    // 获取文件扩展名并转换为小写
    std::string extension = std::filesystem::path(filename).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    std::string directory = std::filesystem::path(filename).parent_path().string();

    // 设置导入参数
    unsigned int flags = aiProcess_Triangulate |   // 三角化所有面
                         aiProcess_GenNormals |    // 生成法线（如果没有）
                         aiProcess_FlipUVs |       // 翻转UV坐标
                         aiProcess_GenUVCoords |   // 生成UV坐标（如果没有）
                         aiProcess_OptimizeMeshes; // 优化网格

    Assimp::Importer importer_;

    // 导入场景
    const aiScene *scene = importer_.ReadFile(filename.c_str(), flags);

    if (!scene)
    {
        LOG_ERROR("Failed to load model: {}", importer_.GetErrorString());
        return std::shared_ptr<Model>();
    }

    if (!scene->HasMeshes())
    {
        LOG_ERROR("Model contains no meshes: {}", filename);
        return std::shared_ptr<Model>();
    }

    // 处理场景
    auto subMeshes = processScene(scene, directory);

    LOG_INFO("Model loaded successfully:");
    LOG_INFO("  Total vertices: {}", totalVertices_);
    LOG_INFO("  Total faces: {}", totalFaces_);
    LOG_INFO("  Total meshes: {}", totalMeshes_);
    // LOG_INFO("  Final mesh vertices: {}", mesh ? mesh->getVertexCount() : 0);
    // LOG_INFO("  Final mesh indices: {}", mesh ? mesh->getIndexCount() : 0);

    return std::make_shared<Model>(subMeshes, directory);
}

std::vector<std::string> AssimpModelLoader::getSupportedFormats()
{
    // Assimp支持的主要格式
    return {".obj", ".fbx", ".dae", ".3ds",  ".blend", ".ply",  ".stl", ".x",   ".gltf", ".glb",  ".3mf",   ".amf",
            ".ase", ".bvh", ".cob", ".csm",  ".dxf",   ".hmp",  ".ifc", ".iqm", ".irr",  ".lwo",  ".lws",   ".md2",
            ".md3", ".md5", ".mdc", ".mdl",  ".mmd",   ".ms3d", ".ndo", ".nff", ".off",  ".ogex", ".q3bsp", ".q3d",
            ".raw", ".sib", ".smd", ".step", ".stp",   ".ter",  ".vta", ".x3d", ".xgl",  ".zgl"};
}

bool AssimpModelLoader::isFormatSupported(const std::string &extension)
{
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    auto formats = getSupportedFormats();
    return std::find(formats.begin(), formats.end(), ext) != formats.end();
}

std::vector<SubMesh> AssimpModelLoader::processScene(const aiScene *scene, const std::string &directory)
{
    std::vector<std::shared_ptr<Material>> materials;
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
    {
        std::shared_ptr<Material> material = std::make_shared<Material>();
        processMaterial(scene->mMaterials[i], scene, directory, material);
        materials.push_back(material);
    }

    for (auto &material : materials)
    {
        materialManager_.AddMaterial(material->getName(), material, true);
    }

    std::vector<SubMesh> subMeshes;

    // 处理根节点，使用单位矩阵作为初始变换
    processNode(scene->mRootNode, scene, subMeshes, Mat4(), materials);
    return subMeshes;
}

void AssimpModelLoader::processNode(aiNode *node, const aiScene *scene, std::vector<SubMesh> &subMeshes,
                                    const Mat4 &parentTransform,
                                    const std::vector<std::shared_ptr<Material>> &materials)
{
    // 计算当前节点的累积变换
    Mat4 nodeTransform = parentTransform * assimpToMatrix4(node->mTransformation);

    // 处理当前节点的所有网格
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        auto meshprt = std::make_shared<Mesh>();
        processMesh(mesh, scene, *meshprt, nodeTransform);
        // 关联材质（若无则传空指针）
        std::shared_ptr<Material> mat =
            materials.size() > mesh->mMaterialIndex ? materials[mesh->mMaterialIndex] : nullptr;
        subMeshes.push_back({meshprt, mat});
        totalMeshes_++;
    }

    // 递归处理子节点
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        processNode(node->mChildren[i], scene, subMeshes, nodeTransform, materials);
    }
}

void AssimpModelLoader::processMesh(aiMesh *mesh, const aiScene *scene, Mesh &outMesh, const Mat4 &transform)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // 处理顶点
    vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex vertex;

        // 位置 - 先转换为我们的向量类型，然后应用变换
        Vector3 position = assimpToVector3(mesh->mVertices[i]);
        position = transform * position; // 应用变换
        vertex.position = position;

        // 法线 - 也需要变换（旋转部分）
        if (mesh->HasNormals())
        {
            Vector3 normal = assimpToVector3(mesh->mNormals[i]);
            // 法线变换只使用旋转缩放部分，不使用位移
            Mat4 rotationScale = transform;
            rotationScale(3, 0) = 0.0f;
            rotationScale(3, 1) = 0.0f;
            rotationScale(3, 2) = 0.0f;
            rotationScale(3, 3) = 1.0f;
            rotationScale(0, 3) = 0.0f;
            rotationScale(1, 3) = 0.0f;
            rotationScale(2, 3) = 0.0f;
            normal = rotationScale * normal;
            normal = VectorUtils::Normalize(normal); // 重新标准化
            vertex.normal = normal;
        }

        // 纹理坐标
        if (mesh->HasTextureCoords(0))
        {
            vertex.texCoord = assimpToVector2(aiVector2D(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y));
        }

        vertices.push_back(vertex);
    }

    // 处理索引（保持不变）
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        if (face.mNumIndices == 3)
        {
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }
    }

    outMesh.setVertices(vertices);
    outMesh.setIndices(indices);
    outMesh.Setup();

    // 更新统计
    totalVertices_ += mesh->mNumVertices;
    totalFaces_ += mesh->mNumFaces;
}

void AssimpModelLoader::processMaterial(aiMaterial *material, const aiScene *scene, const std::string &directory,
                                        std::shared_ptr<Material> &outMaterial)
{
    aiString name;
    if (AI_SUCCESS == material->Get(AI_MATKEY_NAME, name))
    {
        outMaterial->setName(name.C_Str());
        LOG_CORE_INFO("Material name: {}", outMaterial->getName());
    }

    // 漫反射颜色
    aiColor3D diffuse(0.f, 0.f, 0.f);
    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
    {
        outMaterial->setDiffuseColor(assimpToColor3(diffuse));
    }
    LOG_CORE_INFO("Diffuse color: {}, {}, {}", outMaterial->getDiffuseColor().x, outMaterial->getDiffuseColor().y,
                  outMaterial->getDiffuseColor().z);

    // 镜面反射颜色
    aiColor3D specular(0.f, 0.f, 0.f);
    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, specular))
    {
        outMaterial->setSpecularColor(assimpToColor3(specular));
    }
    LOG_CORE_INFO("Specular color: {}, {}, {}", outMaterial->getSpecularColor().x, outMaterial->getSpecularColor().y,
                  outMaterial->getSpecularColor().z);

    // 环境光颜色
    aiColor3D ambient(0.f, 0.f, 0.f);
    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_AMBIENT, ambient))
    {
        outMaterial->setAmbientColor(assimpToColor3(ambient));
    }
    LOG_CORE_INFO("Ambient color: {}, {}, {}", outMaterial->getAmbientColor().x, outMaterial->getAmbientColor().y,
                  outMaterial->getAmbientColor().z);
    // 镜面反射强度
    float shininess = 0.0f;
    if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, shininess))
    {
        outMaterial->setShininess(shininess);
    }
    LOG_CORE_INFO("Shininess: {}", outMaterial->getShininess());
    // // 漫反射纹理
    // aiString diffuseTexture;
    // if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTexture)) {
    //     outMaterial.diffuseTexture = diffuseTexture.C_Str();
    // }
    // LOG_INFO("Diffuse texture: {}", outMaterial.diffuseTexture);
    // // 法线纹理
    // aiString normalTexture;
    // if (AI_SUCCESS == material->GetTexture(aiTextureType_NORMALS, 0, &normalTexture)) {
    //     outMaterial.normalTexture = normalTexture.C_Str();
    // }
    // LOG_INFO("Normal texture: {}", outMaterial.normalTexture);
    // // 镜面反射纹理
    // aiString specularTexture;
    // if (AI_SUCCESS == material->GetTexture(aiTextureType_SPECULAR, 0, &specularTexture)) {
    //     outMaterial.specularTexture = specularTexture.C_Str();
    // }
    // LOG_INFO("Specular texture: {}", outMaterial.specularTexture);

    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
    {
        for (unsigned int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); ++i)
        {
            aiString diffuseTexture;
            if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, i, &diffuseTexture))
            {
                std::shared_ptr<Texture2D> texture =
                    textureManager_.LoadTexture2D(directory + "/" + diffuseTexture.C_Str());
                if (texture)
                {
                    outMaterial->addDiffuseTexture(texture);
                }
                LOG_CORE_INFO("Loaded diffuse texture: {}", diffuseTexture.C_Str());
            }
        }
    }
    else
    {
        outMaterial->addDiffuseTexture(textureManager_.GetDefaultWhiteTexture());
        LOG_CORE_INFO("No diffuse texture found for material: {}, using default white texture", outMaterial->getName());
    }
    if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
    {
        for (unsigned int i = 0; i < material->GetTextureCount(aiTextureType_NORMALS); ++i)
        {
            aiString normalTexture;
            if (AI_SUCCESS == material->GetTexture(aiTextureType_NORMALS, i, &normalTexture))
            {
                std::shared_ptr<Texture2D> texture =
                    textureManager_.LoadTexture2D(directory + "/" + normalTexture.C_Str());
                if (texture)
                {
                    outMaterial->addNormalTexture(texture);
                }
                LOG_CORE_INFO("Loaded normal texture: {}", normalTexture.C_Str());
            }
        }
    }
    else
    {
        outMaterial->addNormalTexture(textureManager_.GetDefaultBlackTexture());
        LOG_CORE_INFO("No normal texture found for material: {}, using default black texture", outMaterial->getName());
    }

    if (material->GetTextureCount(aiTextureType_SPECULAR) > 0)
    {
        for (unsigned int i = 0; i < material->GetTextureCount(aiTextureType_SPECULAR); ++i)
        {
            aiString specularTexture;
            if (AI_SUCCESS == material->GetTexture(aiTextureType_SPECULAR, i, &specularTexture))
            {
                std::shared_ptr<Texture2D> texture =
                    textureManager_.LoadTexture2D(directory + "/" + specularTexture.C_Str());
                if (texture)
                {
                    outMaterial->addSpecularTexture(texture);
                }
                LOG_CORE_INFO("Loaded specular texture: {}", specularTexture.C_Str());
            }
        }
    }
    else
    {
        outMaterial->addSpecularTexture(textureManager_.GetDefaultWhiteTexture());
        LOG_CORE_INFO("No specular texture found for material: {}, using default white texture",
                      outMaterial->getName());
    }
    LOG_CORE_INFO("Processed material: {}", outMaterial->getName());
}

Vector3 AssimpModelLoader::assimpToVector3(const aiVector3D &v)
{
    return Vector3(v.x, v.y, v.z);
}

Vector2 AssimpModelLoader::assimpToVector2(const aiVector2D &v)
{
    return Vector2(v.x, v.y);
}

Vector3 AssimpModelLoader::assimpToColor3(const aiColor3D &c)
{
    return Vector3(c.r, c.g, c.b);
}

// Vector4 AssimpModelLoader::assimpToColor4(const aiColor4D& c) {
//     return Vector4(c.r, c.g, c.b, c.a);
// }

Mat4 AssimpModelLoader::assimpToMatrix4(const aiMatrix4x4 &m)
{
    return Mat4(m.a1, m.a2, m.a3, m.a4, m.b1, m.b2, m.b3, m.b4, m.c1, m.c2, m.c3, m.c4, m.d1, m.d2, m.d3, m.d4);
}

GSENGINE_NAMESPACE_END
