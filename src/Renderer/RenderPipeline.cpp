#include "RenderPipeline.h"
#include "FinalPass.h"
#include "ForwardPass.h"
#include "GeometryPass.h"
#include "LightingPass.h"
#include "PostProcessPass.h"
#include "ShadowPass.h"
#include "SSAOPass.h"
#include "SSAOBlurPass.h"
#include "PostProcessChain.h"
#include "Effects/OutlineEffect.h"
#include "Effects/BloomEffect.h"
#include "RenderContext.h"
#include "ShaderManager.h"
#include "Logger/Log.h"
#include <cstring>
#include <stdexcept>

RENDERER_NAMESPACE_BEGIN

static const std::vector<const char *> s_viewModeLabels = {
    "Final (PostProcess)", "Lighting", "Position", "Normal", "Diffuse", "Specular", "Depth", "SSAO"};

RenderPipeline::RenderPipeline(int width, int height, ShaderManager &shaderManager, const RenderPipelineConfig &config)
    : m_width(width), m_height(height)
{
    // 通过 ShaderManager 统一加载所有内置 Shader
    auto basepassShader =
        shaderManager.LoadShader("basepass", "res/shaders/basepass.vs.glsl", "res/shaders/basepass.fs.glsl");
    auto lambertShader =
        shaderManager.LoadShader("lambert", "res/shaders/lambert.vs.glsl", "res/shaders/lambert.fs.glsl");
    auto pbrShader = shaderManager.LoadShader("pbr", "res/shaders/pbr.vs.glsl", "res/shaders/pbr.fs.glsl");
    auto outlineShader =
        shaderManager.LoadShader("outline", "res/shaders/final.vs.glsl", "res/shaders/outline.fs.glsl");
    auto finalShader = shaderManager.LoadShader("final", "res/shaders/final.vs.glsl", "res/shaders/final.fs.glsl");
    auto shadowShader = shaderManager.LoadShader("shadow", "res/shaders/shadow.vs.glsl", "res/shaders/shadow.fs.glsl");
    auto ssaoShader = shaderManager.LoadShader("ssao", "res/shaders/ssao.vs.glsl", "res/shaders/ssao.fs.glsl");
    auto ssaoBlurShader =
        shaderManager.LoadShader("ssao_blur", "res/shaders/ssao.vs.glsl", "res/shaders/ssao_blur.fs.glsl");
    // Bloom Shaders
    auto bloomThresholdShader =
        shaderManager.LoadShader("bloom_threshold", "res/shaders/final.vs.glsl", "res/shaders/bloom_threshold.fs.glsl");
    auto bloomBlurShader =
        shaderManager.LoadShader("bloom_blur", "res/shaders/final.vs.glsl", "res/shaders/bloom_blur.fs.glsl");
    auto bloomCompositeShader =
        shaderManager.LoadShader("bloom_composite", "res/shaders/final.vs.glsl", "res/shaders/bloom_composite.fs.glsl");

    if (!basepassShader || !lambertShader || !pbrShader || !outlineShader || !finalShader || !bloomThresholdShader ||
        !bloomBlurShader || !bloomCompositeShader || !shadowShader || !ssaoShader || !ssaoBlurShader)
    {
        throw std::runtime_error("RenderPipeline initialization failed: required shader load failed");
    }

    // 按执行顺序构建 Pass 列表
    auto geometry = std::make_unique<GeometryPass>(width, height, basepassShader);
    m_geometryPass = geometry.get();

    m_passes.push_back(std::move(geometry));
    m_passes.push_back(std::make_unique<ShadowPass>(config.shadowMapResolution, shadowShader));
    m_passes.push_back(std::make_unique<SSAOPass>(width, height, ssaoShader));
    m_passes.push_back(std::make_unique<SSAOBlurPass>(width, height, ssaoBlurShader));
    m_passes.push_back(std::make_unique<LightingPass>(width, height, pbrShader));
    m_passes.push_back(std::make_unique<ForwardPass>());

    // 后处理效果链（替代原来的 PostProcessPass）
    auto ppChain = std::make_unique<PostProcessChain>(width, height);
    ppChain->AddEffect(std::make_unique<OutlineEffect>(outlineShader));
    ppChain->AddEffect(
        std::make_unique<BloomEffect>(width, height, bloomThresholdShader, bloomBlurShader, bloomCompositeShader));
    m_passes.push_back(std::move(ppChain));

    auto finalPass = std::make_unique<FinalPass>(width, height, finalShader);
    m_finalPass = finalPass.get();
    m_passes.push_back(std::move(finalPass));
}

RenderPipeline::~RenderPipeline()
{
    for (auto &pass : m_passes)
    {
        if (pass)
            pass.reset();
    }
    m_passes.clear();
    m_forwardRenderables.clear();
    m_forwardShader.reset();
    m_finalPass = nullptr;
    m_geometryPass = nullptr;
    m_width = 0;
    m_height = 0;
    m_exposure = 1.0f;
    m_tonemapMode = 2;
    m_lastDisplayTex = 0;
}

void RenderPipeline::Resize(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;
    if (m_width == width && m_height == height)
        return;

    m_width = width;
    m_height = height;
    for (auto &pass : m_passes)
    {
        if (pass)
            pass->Resize(width, height);
    }
}

void RenderPipeline::Execute(Camera &camera, const std::vector<std::shared_ptr<Renderable>> &sceneRenderables,
                             const std::vector<std::shared_ptr<Light>> &lights, int selectedUID, ViewMode viewMode,
                             float currentTime, bool presentToScreen)
{
    // ---- 1. 填充 RenderContext ----
    RenderContext ctx;
    ctx.camera = &camera;
    ctx.width = m_width;
    ctx.height = m_height;
    ctx.currentTime = currentTime;
    ctx.selectedUID = selectedUID;
    ctx.lights = &lights;
    ctx.sceneRenderables = &sceneRenderables;
    ctx.forwardRenderables = &m_forwardRenderables;
    ctx.forwardShader = m_forwardShader;
    ctx.exposure = m_exposure;
    ctx.tonemapMode = m_tonemapMode;
    ctx.ssaoEnabled = m_ssaoEnabled;
    ctx.ssaoRadius = m_ssaoRadius;
    ctx.ssaoBias = m_ssaoBias;
    ctx.ssaoStrength = m_ssaoStrength;

    // 预计算矩阵
    camera.getViewMatrix(ctx.viewMatrix);
    camera.getPerspectiveMatrix(ctx.projMatrix, 45.0f, static_cast<float>(m_width) / static_cast<float>(m_height),
                                0.01f, 1000.0f);

    // ---- 2. 依次执行除 FinalPass 以外的所有 Pass ----
    for (size_t i = 0; i + 1 < m_passes.size(); ++i)
    {
        m_passes[i]->Execute(ctx);
    }

    // ---- 3. 根据 ViewMode 选择 FinalPass 的输入纹理 ----
    ctx.displaySingleChannelR = false;
    switch (viewMode)
    {
    case ViewMode::Final:
        ctx.displayTex = ctx.postProcessColorTex;
        break;
    case ViewMode::Lighting:
        ctx.displayTex = ctx.lightingTex;
        break;
    case ViewMode::Position:
        ctx.displayTex = ctx.gPositionTex;
        break;
    case ViewMode::Normal:
        ctx.displayTex = ctx.gNormalTex;
        break;
    case ViewMode::Diffuse:
        ctx.displayTex = ctx.gDiffuseTex;
        break;
    case ViewMode::Specular:
        ctx.displayTex = ctx.gSpecularTex;
        break;
    case ViewMode::Depth:
        ctx.displayTex = ctx.gDepthTex;
        break;
    case ViewMode::SSAO:
        ctx.displayTex = ctx.ssaoTex;
        ctx.displaySingleChannelR = true;
        break;
    }

    // ---- 4. 始终执行 FinalPass（ToneMapping + Gamma → 离屏 LDR 纹理）----
    m_passes.back()->Execute(ctx);
    m_lastDisplayTex = ctx.finalTex;

    // ---- 5. 如需直接输出到屏幕（非编辑器模式）----
    if (presentToScreen && m_finalPass)
    {
        m_finalPass->PresentToScreen(m_width, m_height);
    }
}

void RenderPipeline::AddForwardRenderable(const std::shared_ptr<Renderable> &renderable)
{
    AddForwardRenderable(renderable, nullptr, RenderContext::ForwardRenderState{});
}

void RenderPipeline::AddForwardRenderable(const std::shared_ptr<Renderable> &renderable,
                                          const std::shared_ptr<Shader> &shader)
{
    AddForwardRenderable(renderable, shader, RenderContext::ForwardRenderState{});
}

void RenderPipeline::AddForwardRenderable(const std::shared_ptr<Renderable> &renderable,
                                          const std::shared_ptr<Shader> &shader,
                                          const RenderContext::ForwardRenderState &state)
{
    if (!renderable)
        return;
    m_forwardRenderables.push_back({renderable, shader, state});
}

void RenderPipeline::SetForwardShader(const std::shared_ptr<Shader> &shader)
{
    m_forwardShader = shader;
}

int RenderPipeline::PickObject(unsigned int mouseX, unsigned int mouseY)
{
    if (!m_geometryPass)
        return -1;
    return m_geometryPass->GetCurrentSelectedUID(mouseX, mouseY);
}

const std::vector<const char *> &RenderPipeline::GetViewModeLabels()
{
    return s_viewModeLabels;
}

// ---- Pass 管理 API 实现 ----

int RenderPipeline::FindPassIndex(const char *name) const
{
    if (!name)
        return -1;
    for (size_t i = 0; i < m_passes.size(); ++i)
    {
        if (m_passes[i] && std::strcmp(m_passes[i]->GetName(), name) == 0)
            return static_cast<int>(i);
    }
    return -1;
}

bool RenderPipeline::InsertPassAfter(const char *existingPassName, std::unique_ptr<IRenderPass> pass)
{
    if (!pass)
        return false;
    int idx = FindPassIndex(existingPassName);
    if (idx < 0)
    {
        LOG_CORE_WARN("RenderPipeline::InsertPassAfter: pass '{}' not found", existingPassName);
        return false;
    }

    pass->Resize(m_width, m_height);
    m_passes.insert(m_passes.begin() + idx + 1, std::move(pass));
    return true;
}

bool RenderPipeline::InsertPassBefore(const char *existingPassName, std::unique_ptr<IRenderPass> pass)
{
    if (!pass)
        return false;
    int idx = FindPassIndex(existingPassName);
    if (idx < 0)
    {
        LOG_CORE_WARN("RenderPipeline::InsertPassBefore: pass '{}' not found", existingPassName);
        return false;
    }

    pass->Resize(m_width, m_height);
    m_passes.insert(m_passes.begin() + idx, std::move(pass));
    return true;
}

std::unique_ptr<IRenderPass> RenderPipeline::RemovePass(const char *passName)
{
    int idx = FindPassIndex(passName);
    if (idx < 0)
    {
        LOG_CORE_WARN("RenderPipeline::RemovePass: pass '{}' not found", passName);
        return nullptr;
    }

    // 如果移除的是 GeometryPass，清除缓存指针
    if (m_passes[idx].get() == m_geometryPass)
    {
        LOG_CORE_WARN("RenderPipeline::RemovePass: removing GeometryPass, PickObject will be unavailable");
        m_geometryPass = nullptr;
    }

    auto removed = std::move(m_passes[idx]);
    m_passes.erase(m_passes.begin() + idx);
    return removed;
}

std::unique_ptr<IRenderPass> RenderPipeline::ReplacePass(const char *passName, std::unique_ptr<IRenderPass> newPass)
{
    if (!newPass)
        return nullptr;
    int idx = FindPassIndex(passName);
    if (idx < 0)
    {
        LOG_CORE_WARN("RenderPipeline::ReplacePass: pass '{}' not found", passName);
        return nullptr;
    }

    bool wasGeometryPass = (m_passes[idx].get() == m_geometryPass);

    newPass->Resize(m_width, m_height);
    auto old = std::move(m_passes[idx]);
    m_passes[idx] = std::move(newPass);

    // 维护 GeometryPass 缓存指针
    if (wasGeometryPass)
    {
        auto *asGeometry = dynamic_cast<GeometryPass *>(m_passes[idx].get());
        if (asGeometry)
        {
            m_geometryPass = asGeometry;
        }
        else
        {
            LOG_CORE_WARN(
                "RenderPipeline::ReplacePass: new pass is not a GeometryPass, PickObject will be unavailable");
            m_geometryPass = nullptr;
        }
    }

    return old;
}

IRenderPass *RenderPipeline::GetPass(const char *passName) const
{
    int idx = FindPassIndex(passName);
    if (idx < 0)
        return nullptr;
    return m_passes[idx].get();
}

RENDERER_NAMESPACE_END
