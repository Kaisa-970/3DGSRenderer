#pragma once

#include "Core/RenderCore.h"
#include "Camera.h"
#include "GeometryPass.h"
#include "LightingPass.h"
#include "PostProcessPass.h"
#include "ForwardPass.h"
#include "FinalPass.h"
#include "Renderable.h"
#include <memory>
#include <vector>

RENDERER_NAMESPACE_BEGIN

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
/// 将"怎么渲染"从应用层中解耦出来
class RENDERER_API RenderPipeline
{
public:
    RenderPipeline(int width, int height);
    ~RenderPipeline();

    /// 执行完整的延迟渲染管线
    /// @param camera          当前相机
    /// @param sceneRenderables 场景中所有延迟渲染的物体
    /// @param lightPosition   光源位置（临时方案，将来由光源系统管理）
    /// @param selectedUID     当前选中物体的 UID（用于描边高亮）
    /// @param viewMode        G-Buffer 可视化模式
    /// @param currentTime     当前时间（用于前向渲染特效动画）
    void Execute(Camera& camera,
                 const std::vector<std::shared_ptr<Renderable>>& sceneRenderables,
                 const Vector3& lightPosition,
                 unsigned int selectedUID,
                 ViewMode viewMode,
                 float currentTime);

    /// 添加前向渲染物体（半透明/特效物体）
    void AddForwardRenderable(const std::shared_ptr<Renderable>& renderable);

    /// 设置前向渲染使用的 Shader
    void SetForwardShader(const std::shared_ptr<Shader>& shader);

    /// 从 G-Buffer UID 纹理中拾取物体
    unsigned int PickObject(unsigned int mouseX, unsigned int mouseY);

    /// 获取 G-Buffer 可视化模式的标签列表（供 GUI 使用）
    static const std::vector<const char*>& GetViewModeLabels();

private:
    // 各渲染阶段
    std::unique_ptr<GeometryPass> m_geometryPass;
    std::unique_ptr<LightingPass> m_lightingPass;
    std::unique_ptr<PostProcessPass> m_postProcessPass;
    std::unique_ptr<ForwardPass> m_forwardPass;
    std::unique_ptr<FinalPass> m_finalPass;

    // 前向渲染资源
    std::shared_ptr<Shader> m_forwardShader;
    std::vector<std::shared_ptr<Renderable>> m_forwardRenderables;

    // 管线配置
    int m_width;
    int m_height;
};

RENDERER_NAMESPACE_END
