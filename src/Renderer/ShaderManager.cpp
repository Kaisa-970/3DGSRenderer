#include "ShaderManager.h"
#include "Logger/Log.h"

RENDERER_NAMESPACE_BEGIN

ShaderManager::ShaderManager() = default;

ShaderManager::~ShaderManager()
{
    Clear();
}

std::shared_ptr<Shader> ShaderManager::LoadShader(const std::string& name,
                                                   const std::string& vertexPath,
                                                   const std::string& fragmentPath)
{
    // 若已缓存，直接返回
    auto it = m_shaders.find(name);
    if (it != m_shaders.end())
    {
        LOG_CORE_WARN("Shader '{}' already loaded, returning cached version", name);
        return it->second;
    }

    // 编译并缓存
    LOG_CORE_INFO("Loading shader '{}': vs={}, fs={}", name, vertexPath, fragmentPath);
    auto shader = Shader::fromFiles(vertexPath, fragmentPath);
    if (shader)
    {
        m_shaders[name] = shader;
    }
    else
    {
        LOG_CORE_ERROR("Failed to load shader '{}'", name);
    }
    return shader;
}

std::shared_ptr<Shader> ShaderManager::GetShader(const std::string& name) const
{
    auto it = m_shaders.find(name);
    if (it != m_shaders.end())
    {
        return it->second;
    }
    LOG_CORE_ERROR("Shader not found: {}", name);
    return nullptr;
}

bool ShaderManager::HasShader(const std::string& name) const
{
    return m_shaders.find(name) != m_shaders.end();
}

void ShaderManager::RemoveShader(const std::string& name)
{
    auto it = m_shaders.find(name);
    if (it != m_shaders.end())
    {
        m_shaders.erase(it);
    }
}

void ShaderManager::Clear()
{
    m_shaders.clear();
}

RENDERER_NAMESPACE_END
