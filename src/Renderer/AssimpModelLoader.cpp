#include "AssimpModelLoader.h"
#include "Logger/Log.h"
#include <algorithm>
#include <filesystem>

RENDERER_NAMESPACE_BEGIN

AssimpModelLoader::AssimpModelLoader()
    : totalVertices_(0)
    , totalFaces_(0)
    , totalMeshes_(0)
{
}

AssimpModelLoader::~AssimpModelLoader() {
}

std::vector<std::shared_ptr<Mesh>> AssimpModelLoader::loadModel(const std::string& filename) {
    LOG_INFO("Loading model with Assimp: {}", filename);

    // 重置统计信息
    totalVertices_ = 0;
    totalFaces_ = 0;
    totalMeshes_ = 0;

    // 检查文件是否存在
    if (!std::filesystem::exists(filename)) {
        LOG_ERROR("Model file does not exist: {}", filename);
        return std::vector<std::shared_ptr<Mesh>>();
    }

    // 获取文件扩展名并转换为小写
    std::string extension = std::filesystem::path(filename).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // 设置导入参数
    unsigned int flags = aiProcess_Triangulate |           // 三角化所有面
                        aiProcess_GenNormals |            // 生成法线（如果没有）
                        aiProcess_CalcTangentSpace |      // 计算切线空间
                        aiProcess_JoinIdenticalVertices | // 合并相同顶点
                        aiProcess_SortByPType |          // 按类型排序
                        aiProcess_FlipUVs |              // 翻转UV坐标
                        aiProcess_GenUVCoords |          // 生成UV坐标（如果没有）
                        aiProcess_OptimizeMeshes |       // 优化网格
                        aiProcess_RemoveRedundantMaterials | // 移除冗余材质
                        aiProcess_FindInvalidData;       // 查找无效数据

    // 导入场景
    const aiScene* scene = importer_.ReadFile(filename.c_str(), flags);

    if (!scene) {
        LOG_ERROR("Failed to load model: {}", importer_.GetErrorString());
        return std::vector<std::shared_ptr<Mesh>>();
    }

    if (!scene->HasMeshes()) {
        LOG_ERROR("Model contains no meshes: {}", filename);
        return std::vector<std::shared_ptr<Mesh>>();
    }

    // 处理场景
    auto meshes = processScene(scene, filename);

    LOG_INFO("Model loaded successfully:");
    LOG_INFO("  Total vertices: {}", totalVertices_);
    LOG_INFO("  Total faces: {}", totalFaces_);
    LOG_INFO("  Total meshes: {}", totalMeshes_);
    // LOG_INFO("  Final mesh vertices: {}", mesh ? mesh->getVertexCount() : 0);
    // LOG_INFO("  Final mesh indices: {}", mesh ? mesh->getIndexCount() : 0);

    return meshes;
}

std::vector<std::string> AssimpModelLoader::getSupportedFormats() {
    // Assimp支持的主要格式
    return {
        ".obj", ".fbx", ".dae", ".3ds", ".blend", ".ply", ".stl",
        ".x", ".gltf", ".glb", ".3mf", ".amf", ".ase", ".bvh", ".cob",
        ".csm", ".dxf", ".hmp", ".ifc", ".iqm", ".irr", ".lwo", ".lws",
        ".md2", ".md3", ".md5", ".mdc", ".mdl", ".mmd", ".ms3d", ".ndo",
        ".nff", ".off", ".ogex", ".q3bsp", ".q3d", ".raw", ".sib", ".smd",
        ".step", ".stp", ".ter", ".vta", ".x3d", ".xgl", ".zgl"
    };
}

bool AssimpModelLoader::isFormatSupported(const std::string& extension) {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    auto formats = getSupportedFormats();
    return std::find(formats.begin(), formats.end(), ext) != formats.end();
}

std::vector<std::shared_ptr<Mesh>> AssimpModelLoader::processScene(const aiScene* scene, const std::string& filename) {
    std::vector<Material> materials;
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        Material material;
        processMaterial(scene->mMaterials[i], scene, material);
        // mesh->addMaterial(material);
        materials.push_back(material);
    }
    
    std::vector<std::shared_ptr<Mesh>> meshes;

    // for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
    //     auto mesh = std::make_shared<Mesh>();
    //     processMesh(scene->mMeshes[i], scene, *mesh, Matrix4(), materials);
    //     meshes.push_back(mesh);
    // }
    // 处理根节点，使用单位矩阵作为初始变换
    processNode(scene->mRootNode, scene, meshes, Matrix4(), materials);
    return meshes;
}
    // auto mesh = std::make_shared<Mesh>();

    // // 处理材质（需要在处理网格之前，因为网格会引用材质索引）
    // for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
    //     Material material;
    //     processMaterial(scene->mMaterials[i], scene, material);
    //     mesh->addMaterial(material);
    // }

    // // 处理根节点，使用单位矩阵作为初始变换
    // processNode(scene->mRootNode, scene, *mesh, Matrix4());

    // return mesh;
// }

void AssimpModelLoader::processNode(aiNode* node, const aiScene* scene, std::vector<std::shared_ptr<Mesh>>& meshes, const Matrix4& parentTransform, const std::vector<Material>& materials) 
{
    // 计算当前节点的累积变换
    Matrix4 nodeTransform = parentTransform * assimpToMatrix4(node->mTransformation);
    
    // 处理当前节点的所有网格
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        auto meshprt = std::make_shared<Mesh>();
        processMesh(mesh, scene, *meshprt, nodeTransform, materials);
        meshes.push_back(meshprt);
        totalMeshes_++;
    }

    // 递归处理子节点
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], scene, meshes, nodeTransform, materials);
    }
}

void AssimpModelLoader::processMesh(aiMesh* mesh, const aiScene* scene, Mesh& outMesh, const Matrix4& transform, const std::vector<Material>& materials) {
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;

    // 处理顶点
    vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        MeshVertex vertex;

        // 位置 - 先转换为我们的向量类型，然后应用变换
        Vector3 position = assimpToVector3(mesh->mVertices[i]);
        position = transform * position;  // 应用变换
        vertex.position = position;

        // 法线 - 也需要变换（旋转部分）
        if (mesh->HasNormals()) {
            Vector3 normal = assimpToVector3(mesh->mNormals[i]);
            // 法线变换只使用旋转缩放部分，不使用位移
            Matrix4 rotationScale = transform;
            rotationScale.m[12] = rotationScale.m[13] = rotationScale.m[14] = 0.0f; // 移除位移
            rotationScale.m[3] = rotationScale.m[7] = rotationScale.m[11] = 0.0f; // 确保最后一行正确
            rotationScale.m[15] = 1.0f;
            normal = rotationScale * normal;
            normal = normal.normalized(); // 重新标准化
            vertex.normal = normal;
        }

        // 纹理坐标
        if (mesh->HasTextureCoords(0)) {
            vertex.texCoord = assimpToVector2(aiVector2D(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            ));
        }

        // 切线和副切线（如果有的话也需要变换）
        if (mesh->HasTangentsAndBitangents()) {
            Vector3 tangent = assimpToVector3(mesh->mTangents[i]);
            tangent = transform * tangent;
            vertex.tangent = tangent.normalized();
            
            Vector3 bitangent = assimpToVector3(mesh->mBitangents[i]);
            bitangent = transform * bitangent;
            vertex.bitangent = bitangent.normalized();
        }

        vertices.push_back(vertex);
    }

    // 处理索引（保持不变）
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        if (face.mNumIndices == 3) {
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }
    }

    // 创建子网格
    // SubMesh subMesh;
    // subMesh.name = mesh->mName.C_Str();
    // if (subMesh.name.empty()) {
    //     subMesh.name = "mesh_" + std::to_string(totalMeshes_);
    // }
    // subMesh.indices = indices;
    // subMesh.materialIndex = mesh->mMaterialIndex;
    outMesh.setVertices(vertices);
    outMesh.setIndices(indices);
    outMesh.addMaterial(materials[mesh->mMaterialIndex]);

    // // 添加到主网格
    // outMesh.appendVertices(vertices);
    // outMesh.addSubMesh(subMesh);

    // 更新统计
    totalVertices_ += mesh->mNumVertices;
    totalFaces_ += mesh->mNumFaces;
}

void AssimpModelLoader::processMaterial(aiMaterial* material, const aiScene* scene, Material& outMaterial) {
    aiString name;
    if (AI_SUCCESS == material->Get(AI_MATKEY_NAME, name)) {
        outMaterial.name = name.C_Str();
    }

    // 漫反射颜色
    aiColor3D diffuse(0.f, 0.f, 0.f);
    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse)) {
        outMaterial.diffuseColor = assimpToColor3(diffuse);
    }
    LOG_INFO("Diffuse color: {}, {}, {}", outMaterial.diffuseColor.x, outMaterial.diffuseColor.y, outMaterial.diffuseColor.z);

    // 镜面反射颜色
    aiColor3D specular(0.f, 0.f, 0.f);
    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, specular)) {
        outMaterial.specularColor = assimpToColor3(specular);
    }
    LOG_INFO("Specular color: {}, {}, {}", outMaterial.specularColor.x, outMaterial.specularColor.y, outMaterial.specularColor.z);

    // 环境光颜色
    aiColor3D ambient(0.f, 0.f, 0.f);
    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_AMBIENT, ambient)) {
        outMaterial.ambientColor = assimpToColor3(ambient);
    }
    LOG_INFO("Ambient color: {}, {}, {}", outMaterial.ambientColor.x, outMaterial.ambientColor.y, outMaterial.ambientColor.z);
    // 镜面反射强度
    float shininess = 0.0f;
    if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, shininess)) {
        outMaterial.shininess = shininess;
    }
    LOG_INFO("Shininess: {}", outMaterial.shininess);
    // 漫反射纹理
    aiString diffuseTexture;
    if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTexture)) {
        outMaterial.diffuseTexture = diffuseTexture.C_Str();
    }
    LOG_INFO("Diffuse texture: {}", outMaterial.diffuseTexture);
    // 法线纹理
    aiString normalTexture;
    if (AI_SUCCESS == material->GetTexture(aiTextureType_NORMALS, 0, &normalTexture)) {
        outMaterial.normalTexture = normalTexture.C_Str();
    }
    LOG_INFO("Normal texture: {}", outMaterial.normalTexture);
    // 镜面反射纹理
    aiString specularTexture;
    if (AI_SUCCESS == material->GetTexture(aiTextureType_SPECULAR, 0, &specularTexture)) {
        outMaterial.specularTexture = specularTexture.C_Str();
    }
    LOG_INFO("Specular texture: {}", outMaterial.specularTexture);
}

Vector3 AssimpModelLoader::assimpToVector3(const aiVector3D& v) {
    return Vector3(v.x, v.y, v.z);
}

Vector2 AssimpModelLoader::assimpToVector2(const aiVector2D& v) {
    return Vector2(v.x, v.y);
}

Vector3 AssimpModelLoader::assimpToColor3(const aiColor3D& c) {
    return Vector3(c.r, c.g, c.b);
}

// Vector4 AssimpModelLoader::assimpToColor4(const aiColor4D& c) {
//     return Vector4(c.r, c.g, c.b, c.a);
// }

Matrix4 AssimpModelLoader::assimpToMatrix4(const aiMatrix4x4& m) {
    return Matrix4(
        m.a1, m.a2, m.a3, m.a4,
        m.b1, m.b2, m.b3, m.b4,
        m.c1, m.c2, m.c3, m.c4,
        m.d1, m.d2, m.d3, m.d4
    );
}

RENDERER_NAMESPACE_END
