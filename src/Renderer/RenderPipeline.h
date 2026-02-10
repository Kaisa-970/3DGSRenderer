#pragma once

#include "Core/RenderCore.h"
#include "IRenderPass.h"
#include "RenderContext.h"
#include "Camera.h"
#include "Light.h"
#include "Renderable.h"
#include <memory>
#include <vector>

RENDERER_NAMESPACE_BEGIN

class GeometryPass;  // 前向声明（用于 PickObject 特有功能）
class ShaderManager; // 前向声明

/// G-Buffer 可视化模式
enum class ViewMode
{
    Final = 0,
    Lighting,
    Position,
    Normal,
    Diffuse,
    Specular,
    Shininess,
    Depth,
};

/// 渲染管线：统一编排所有 RenderPass 的执行顺序
/// 通过 RenderContext 在 Pass 之间传递数据，形成清晰的数据流
class RENDERER_API RenderPipeline
{
public:
    RenderPipeline(int width, int height, ShaderManager &shaderManager);
    ~RenderPipeline();

    // 禁止拷贝（vector<unique_ptr> 不可拷贝，MSVC dllexport 要求显式声明）
    RenderPipeline(const RenderPipeline &) = delete;
    RenderPipeline &operator=(const RenderPipeline &) = delete;

    /// 执行完整的延迟渲染管线
    void Execute(Camera &camera, const std::vector<std::shared_ptr<Renderable>> &sceneRenderables, const Light &light,
                 int selectedUID, ViewMode viewMode, float currentTime);

    /// 添加前向渲染物体（半透明/特效物体），使用默认 forward shader（若已设置）
    void AddForwardRenderable(const std::shared_ptr<Renderable> &renderable);
    /// 添加前向渲染物体，并为该物体指定独立 shader（用于 App 层快速试验特效）
    void AddForwardRenderable(const std::shared_ptr<Renderable> &renderable, const std::shared_ptr<Shader> &shader);
    /// 添加前向渲染物体，指定独立 shader + 渲染状态（混合/深度/剔除）
    void AddForwardRenderable(const std::shared_ptr<Renderable> &renderable,
                             const std::shared_ptr<Shader> &shader,
                             const RenderContext::ForwardRenderState &state);

    /// 设置默认前向渲染 Shader（当单个物体未指定独立 shader 时作为回退）
    void SetForwardShader(const std::shared_ptr<Shader> &shader);

    /// 从 G-Buffer UID 纹理中拾取物体
    int PickObject(unsigned int mouseX, unsigned int mouseY);

    /// 获取 G-Buffer 可视化模式的标签列表（供 GUI 使用）
    static const std::vector<const char *> &GetViewModeLabels();

private:
    // Pass 列表（按执行顺序排列）
    std::vector<std::unique_ptr<IRenderPass>> m_passes;

    // GeometryPass 的裸指针引用（用于 PickObject 等特有功能，生命周期由 m_passes 管理）
    GeometryPass *m_geometryPass = nullptr;

    // 前向渲染资源
    std::shared_ptr<Shader> m_forwardShader;
    std::vector<RenderContext::ForwardRenderItem> m_forwardRenderables;

    // 管线配置
    int m_width;
    int m_height;
};

RENDERER_NAMESPACE_END
