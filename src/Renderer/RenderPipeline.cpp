#include "RenderPipeline.h"

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
    // ---- 准备矩阵 ----
    float viewMatrix[16];
    float projMatrix[16];
    camera.getViewMatrix(viewMatrix);
    camera.getPerspectiveMatrix(projMatrix, 45.0f,
                                static_cast<float>(m_width) / static_cast<float>(m_height),
                                0.01f, 1000.0f);

    // ---- 设置前向渲染 shader uniform ----
    if (m_forwardShader)
    {
        float vx, vy, vz;
        camera.getPosition(vx, vy, vz);
        m_forwardShader->use();
        m_forwardShader->setVec3("u_viewPos", vx, vy, vz);
        m_forwardShader->unuse();
    }

    // ---- 1. Geometry Pass: 将场景物体渲染到 G-Buffer ----
    m_geometryPass->Begin(viewMatrix, projMatrix);
    for (const auto& renderable : sceneRenderables)
    {
        if (renderable)
            m_geometryPass->Render(renderable.get());
    }
    m_geometryPass->End();

    // ---- 2. Lighting Pass: 使用 G-Buffer 计算光照 ----
    m_lightingPass->Begin(camera, light);
    m_lightingPass->Render(
        m_geometryPass->getPositionTexture(),
        m_geometryPass->getNormalTexture(),
        m_geometryPass->getDiffuseTexture(),
        m_geometryPass->getSpecularTexture(),
        m_geometryPass->getShininessTexture()
    );
    m_lightingPass->End();

    // ---- 3. Post-Process Pass: 后处理（描边高亮等） ----
    m_postProcessPass->render(
        m_width, m_height, camera, selectedUID,
        m_geometryPass->getUIDTexture(),
        m_geometryPass->getPositionTexture(),
        m_geometryPass->getNormalTexture(),
        m_lightingPass->getLightingTexture(),
        m_geometryPass->getDepthTexture()
    );

    // ---- 4. Forward Pass: 半透明/特效物体 ----
    if (m_forwardShader && !m_forwardRenderables.empty())
    {
        m_forwardPass->Render(
            m_width, m_height,
            viewMatrix, projMatrix,
            m_postProcessPass->getColorTexture(),
            m_geometryPass->getDepthTexture(),
            m_forwardRenderables,
            m_forwardShader,
            currentTime
        );
    }

    // ---- 5. Final Pass: 根据 ViewMode 选择输出纹理并渲染到屏幕 ----
    unsigned int displayTex = m_postProcessPass->getColorTexture();
    switch (viewMode)
    {
    case ViewMode::Final:     displayTex = m_postProcessPass->getColorTexture();    break;
    case ViewMode::Lighting:  displayTex = m_lightingPass->getLightingTexture();     break;
    case ViewMode::Position:  displayTex = m_geometryPass->getPositionTexture();    break;
    case ViewMode::Normal:    displayTex = m_geometryPass->getNormalTexture();      break;
    case ViewMode::Diffuse:   displayTex = m_geometryPass->getDiffuseTexture();    break;
    case ViewMode::Specular:  displayTex = m_geometryPass->getSpecularTexture();   break;
    case ViewMode::Shininess: displayTex = m_geometryPass->getShininessTexture();  break;
    case ViewMode::Depth:     displayTex = m_geometryPass->getDepthTexture();      break;
    }

    m_finalPass->render(m_width, m_height, displayTex);
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
