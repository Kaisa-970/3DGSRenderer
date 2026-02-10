#pragma once

#include "Core/RenderCore.h"
#include "Shader.h"
#include <map>
#include <string>
#include <memory>

RENDERER_NAMESPACE_BEGIN

/// Shader 资源管理器：统一加载、缓存和检索 Shader 程序
/// 避免同一 Shader 被重复编译，所有 Shader 的生命周期由此类集中管理
class RENDERER_API ShaderManager
{
public:
    ShaderManager();
    ~ShaderManager();

    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;

    /// 从文件加载 Shader 并缓存，若已存在同名 Shader 则直接返回缓存
    std::shared_ptr<Shader> LoadShader(const std::string& name,
                                       const std::string& vertexPath,
                                       const std::string& fragmentPath);

    /// 按名称获取已加载的 Shader
    std::shared_ptr<Shader> GetShader(const std::string& name) const;

    /// 检查是否已存在指定名称的 Shader
    bool HasShader(const std::string& name) const;

    /// 移除指定名称的 Shader
    void RemoveShader(const std::string& name);

    /// 清空所有缓存的 Shader
    void Clear();

private:
    std::map<std::string, std::shared_ptr<Shader>> m_shaders;
};

RENDERER_NAMESPACE_END
