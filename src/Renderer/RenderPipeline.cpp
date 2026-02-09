#include "RenderPipeline.h"
#include "RenderContext.h"

RENDERER_NAMESPACE_BEGIN

static const std::vector<const char*> s_viewModeLabels = {
    "Final (PostProcess)", "Lighting", "Position", "Normal",
    "Diffuse", "Specular", "Shininess", "Depth"
};

RenderPipeline::RenderPipeline(int width, int height)
    : m_width(width), m_height(height)
{
    m_geometryPass    = std::make_unique<GeometryPass>(width, height);
    m_lightingPass    = std::make_unique<LightingPass>(width, height);
    m_postProcessPass = std::make_unique<PostProcessPass>(width, height);
    m_forwardPass     = std::make_unique<ForwardPass>();
    m_finalPass       = std::make_unique<FinalPass>();
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

    // ---- 2. 依次执行各 Pass（每个 Pass 从 ctx 读取输入，写入输出）----
    m_geometryPass->Execute(ctx);     // → ctx.gPositionTex, gNormalTex, ...
    m_lightingPass->Execute(ctx);     // → ctx.lightingTex
    m_postProcessPass->Execute(ctx);  // → ctx.postProcessColorTex
    m_forwardPass->Execute(ctx);      // 渲染到 ctx.postProcessColorTex 上

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

    m_finalPass->Execute(ctx);        // 输出到屏幕
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
