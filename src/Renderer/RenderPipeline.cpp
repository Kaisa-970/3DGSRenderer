#include "RenderPipeline.h"
#include "RenderContext.h"
#include "ShaderManager.h"
#include "GeometryPass.h"
#include "LightingPass.h"
#include "PostProcessPass.h"
#include "ForwardPass.h"
#include "FinalPass.h"

RENDERER_NAMESPACE_BEGIN

static const std::vector<const char*> s_viewModeLabels = {
    "Final (PostProcess)", "Lighting", "Position", "Normal",
    "Diffuse", "Specular", "Shininess", "Depth"
};

RenderPipeline::RenderPipeline(int width, int height, ShaderManager& shaderManager)
    : m_width(width), m_height(height)
{
    // 通过 ShaderManager 统一加载所有内置 Shader
    auto basepassShader    = shaderManager.LoadShader("basepass",    "res/shaders/basepass.vs.glsl",    "res/shaders/basepass.fs.glsl");
    auto lambertShader     = shaderManager.LoadShader("lambert",     "res/shaders/lambert.vs.glsl",     "res/shaders/lambert.fs.glsl");
    auto postprocessShader = shaderManager.LoadShader("postprocess", "res/shaders/final.vs.glsl",       "res/shaders/postprocess.fs.glsl");
    auto finalShader       = shaderManager.LoadShader("final",       "res/shaders/final.vs.glsl",       "res/shaders/final.fs.glsl");

    // 按执行顺序构建 Pass 列表，注入各自的 Shader
    auto geometry    = std::make_unique<GeometryPass>(width, height, basepassShader);
    m_geometryPass   = geometry.get();  // 保留裸指针用于 PickObject

    m_passes.push_back(std::move(geometry));
    m_passes.push_back(std::make_unique<LightingPass>(width, height, lambertShader));
    m_passes.push_back(std::make_unique<PostProcessPass>(width, height, postprocessShader));
    m_passes.push_back(std::make_unique<ForwardPass>());
    m_passes.push_back(std::make_unique<FinalPass>(finalShader));
}

RenderPipeline::~RenderPipeline() = default;

void RenderPipeline::Execute(Camera& camera,
                              const std::vector<std::shared_ptr<Renderable>>& sceneRenderables,
                              const Light& light,
                              unsigned int selectedUID,
                              ViewMode viewMode,
                              float currentTime)
{
    // ---- 1. 填充 RenderContext ----
    RenderContext ctx;
    ctx.camera            = &camera;
    ctx.width             = m_width;
    ctx.height            = m_height;
    ctx.currentTime       = currentTime;
    ctx.selectedUID       = selectedUID;
    ctx.light             = &light;
    ctx.sceneRenderables  = &sceneRenderables;
    ctx.forwardRenderables = &m_forwardRenderables;
    ctx.forwardShader     = m_forwardShader;

    // 预计算矩阵
    camera.getViewMatrix(ctx.viewMatrix);
    camera.getPerspectiveMatrix(ctx.projMatrix, 45.0f,
                                static_cast<float>(m_width) / static_cast<float>(m_height),
                                0.01f, 1000.0f);

    // ---- 2. 依次执行前 4 个 Pass（Geometry → Lighting → PostProcess → Forward）----
    for (size_t i = 0; i + 1 < m_passes.size(); ++i)
    {
        m_passes[i]->Execute(ctx);
    }

    // ---- 3. 根据 ViewMode 选择显示纹理 ----
    switch (viewMode)
    {
    case ViewMode::Final:     ctx.displayTex = ctx.postProcessColorTex; break;
    case ViewMode::Lighting:  ctx.displayTex = ctx.lightingTex;         break;
    case ViewMode::Position:  ctx.displayTex = ctx.gPositionTex;        break;
    case ViewMode::Normal:    ctx.displayTex = ctx.gNormalTex;          break;
    case ViewMode::Diffuse:   ctx.displayTex = ctx.gDiffuseTex;         break;
    case ViewMode::Specular:  ctx.displayTex = ctx.gSpecularTex;        break;
    case ViewMode::Shininess: ctx.displayTex = ctx.gShininessTex;       break;
    case ViewMode::Depth:     ctx.displayTex = ctx.gDepthTex;           break;
    }

    // ---- 4. 最后一个 Pass（FinalPass）输出到屏幕 ----
    m_passes.back()->Execute(ctx);
}

void RenderPipeline::AddForwardRenderable(const std::shared_ptr<Renderable>& renderable)
{
    m_forwardRenderables.push_back(renderable);
}

void RenderPipeline::SetForwardShader(const std::shared_ptr<Shader>& shader)
{
    m_forwardShader = shader;
}

unsigned int RenderPipeline::PickObject(unsigned int mouseX, unsigned int mouseY)
{
    return m_geometryPass->getCurrentSelectedUID(mouseX, mouseY);
}

const std::vector<const char*>& RenderPipeline::GetViewModeLabels()
{
    return s_viewModeLabels;
}

RENDERER_NAMESPACE_END
