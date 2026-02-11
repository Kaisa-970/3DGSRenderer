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
class FinalPass;     // 前向声明（用于 PresentToScreen）
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
                 int selectedUID, ViewMode viewMode, float currentTime, bool presentToScreen = true);
    /// 动态调整渲染尺寸（用于编辑器 Scene 面板变化）
    void Resize(int width, int height);

    /// 添加前向渲染物体（半透明/特效物体），使用默认 forward shader（若已设置）
    void AddForwardRenderable(const std::shared_ptr<Renderable> &renderable);
    /// 添加前向渲染物体，并为该物体指定独立 shader（用于 App 层快速试验特效）
    void AddForwardRenderable(const std::shared_ptr<Renderable> &renderable, const std::shared_ptr<Shader> &shader);
    /// 添加前向渲染物体，指定独立 shader + 渲染状态（混合/深度/剔除）
    void AddForwardRenderable(const std::shared_ptr<Renderable> &renderable, const std::shared_ptr<Shader> &shader,
                              const RenderContext::ForwardRenderState &state);

    /// 设置默认前向渲染 Shader（当单个物体未指定独立 shader 时作为回退）
    void SetForwardShader(const std::shared_ptr<Shader> &shader);

    /// 从 G-Buffer UID 纹理中拾取物体
    int PickObject(unsigned int mouseX, unsigned int mouseY);

    // ---- HDR / Tone Mapping 控制 ----
    void SetExposure(float exposure) { m_exposure = exposure; }
    float GetExposure() const { return m_exposure; }
    void SetTonemapMode(int mode) { m_tonemapMode = mode; }
    int GetTonemapMode() const { return m_tonemapMode; }

    /// 获取 G-Buffer 可视化模式的标签列表（供 GUI 使用）
    static const std::vector<const char *> &GetViewModeLabels();
    /// 获取上一帧根据 ViewMode 选择的显示纹理（供编辑器 Scene View 使用）
    unsigned int GetLastDisplayTexture() const
    {
        return m_lastDisplayTex;
    }
    int GetRenderWidth() const
    {
        return m_width;
    }
    int GetRenderHeight() const
    {
        return m_height;
    }

    // ---- Pass 管理 API ----

    /// 在指定名称的 Pass 之后插入新 Pass（自动同步当前渲染尺寸）
    bool InsertPassAfter(const char *existingPassName, std::unique_ptr<IRenderPass> pass);
    /// 在指定名称的 Pass 之前插入新 Pass（自动同步当前渲染尺寸）
    bool InsertPassBefore(const char *existingPassName, std::unique_ptr<IRenderPass> pass);
    /// 按名称移除 Pass，返回被移除的 Pass（失败返回 nullptr）
    std::unique_ptr<IRenderPass> RemovePass(const char *passName);
    /// 按名称替换 Pass，返回被替换的旧 Pass（失败返回 nullptr）
    std::unique_ptr<IRenderPass> ReplacePass(const char *passName, std::unique_ptr<IRenderPass> newPass);
    /// 按名称查找 Pass（返回裸指针用于运行时配置，生命周期由管线管理）
    IRenderPass *GetPass(const char *passName) const;
    /// 获取当前 Pass 数量
    size_t GetPassCount() const
    {
        return m_passes.size();
    }

private:
    // Pass 列表（按执行顺序排列）
    std::vector<std::unique_ptr<IRenderPass>> m_passes;

    // 特殊 Pass 的裸指针引用（生命周期由 m_passes 管理）
    GeometryPass *m_geometryPass = nullptr; // 用于 PickObject
    FinalPass *m_finalPass = nullptr;       // 用于 PresentToScreen

    // 前向渲染资源
    std::shared_ptr<Shader> m_forwardShader;
    std::vector<RenderContext::ForwardRenderItem> m_forwardRenderables;

    // 管线配置
    int m_width;
    int m_height;
    unsigned int m_lastDisplayTex = 0;

    // HDR / Tone Mapping 参数
    float m_exposure = 1.0f;
    int m_tonemapMode = 2; // 0 = None, 1 = Reinhard, 2 = ACES Filmic

    // Pass 查找辅助
    int FindPassIndex(const char *name) const;
};

RENDERER_NAMESPACE_END
