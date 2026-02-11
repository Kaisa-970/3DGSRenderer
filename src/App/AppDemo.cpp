#include "AppDemo.h"
#include "Assets/MaterialManager.h"
#include "Logger/Log.h"
#include "ModelLoader/AssimpModelLoader.h"
#include "Renderer/Light.h"
#include "Renderer/MathUtils/Random.h"
#include "Renderer/Primitives/CubePrimitive.h"
#include "Renderer/Primitives/QuadPrimitive.h"
#include "Renderer/Primitives/SpherePrimitive.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/Renderable.h"
#include "Renderer/ShaderManager.h"
#include <memory>

using namespace GSEngine;

constexpr float DEG_TO_RAD = 3.1415926f / 180.0f;
#define DEG2RAD(x) (x * DEG_TO_RAD)

#ifdef RENDERER_DEBUG
std::string modelPath = "./res/backpack/backpack.obj";
std::string model2Path = "./res/houtou.fbx";
#endif

// AppDemo的私有成员变量
class AppDemo::Impl
{
public:
    // 渲染管线
    std::unique_ptr<Renderer::RenderPipeline> renderPipeline;

    // 场景光源（作为 Scene 的一部分管理）
    std::shared_ptr<Renderer::Light> mainLight;

    // 光源的可视化球体
    std::shared_ptr<Renderer::Renderable> lightSphereRenderable;

    // GUI状态
    int gbufferViewMode = static_cast<int>(Renderer::ViewMode::Final);
    float exposure = 1.0f;
    int tonemapMode = 2; // 默认 ACES Filmic

    // 应用状态
    int currentSelectedUID = -1;
    std::shared_ptr<Renderer::Renderable> selectedRenderable = nullptr;
    float currentTime = 0.0f;
};

AppDemo::AppDemo(AppConfig config) : Application(config), pImpl(std::make_unique<Impl>())
{
}

AppDemo::~AppDemo() = default;

bool AppDemo::OnInit()
{
    LOG_INFO("AppDemo 初始化中...");

    // 设置GUI - 使用 RenderPipeline 提供的标签列表
    m_guiLayer->SetGBufferViewModes(&pImpl->gbufferViewMode, Renderer::RenderPipeline::GetViewModeLabels());
    m_guiLayer->SetHDRControls(&pImpl->exposure, &pImpl->tonemapMode);
    m_guiLayer->SetScene(m_scene);
    m_guiLayer->SetMaterialManager(m_materialManager);

    // 加载模型（注入资源管理器）
    AssimpModelLoader modelLoader(*m_textureManager, *m_materialManager);
    std::shared_ptr<Renderer::Model> loadedModel = modelLoader.loadModel(modelPath);
    std::shared_ptr<Renderer::Model> loadedModel2 = modelLoader.loadModel(model2Path);

    // 创建场景光源
    pImpl->mainLight =
        std::make_shared<Renderer::Light>(Renderer::Light::CreatePointLight(Renderer::Vector3(0.0f, 5.0f, 0.0f)));
    m_scene->AddLight(pImpl->mainLight);

    // 初始化渲染管线（传入 ShaderManager，内部统一加载 Shader）
    pImpl->renderPipeline =
        std::make_unique<Renderer::RenderPipeline>(m_appConfig.width, m_appConfig.height, *m_shaderManager);

    // 通过 ShaderManager 加载前向渲染 Shader
    auto forwardEffectShader = m_shaderManager->LoadShader("forward_effect", "res/shaders/forward_effect.vs.glsl",
                                                           "res/shaders/forward_effect.fs.glsl");
    pImpl->renderPipeline->SetForwardShader(forwardEffectShader);

    // 创建几何体
    auto cubePrimitive = std::make_shared<Renderer::CubePrimitive>(1.0f);
    auto spherePrimitive = std::make_shared<Renderer::SpherePrimitive>(1.0f, 64, 32);
    auto quadPrimitive = std::make_shared<Renderer::QuadPrimitive>(10.0f);
    auto defaultMaterial = m_materialManager->GetDefaultMaterial();

    // 创建场景内容
    SetupScene(cubePrimitive, spherePrimitive, quadPrimitive, defaultMaterial, loadedModel, loadedModel2);

    LOG_INFO("AppDemo 初始化完成");
    return true;
}

void AppDemo::OnShutdown()
{
    if (!pImpl)
        return;

    // 在 GLFW 终止前释放依赖 GL 上下文的渲染资源
    pImpl->renderPipeline.reset();
    pImpl->lightSphereRenderable.reset();
    pImpl->selectedRenderable.reset();
    pImpl->mainLight.reset();
}

void AppDemo::OnUpdate(float deltaTime)
{
    pImpl->currentTime += deltaTime;

    // 更新光源位置（直接修改 Light 对象）
    float curX = 5.0f * std::sin(pImpl->currentTime);
    float curZ = 5.0f * std::cos(pImpl->currentTime);
    pImpl->mainLight->position = Renderer::Vector3(curX, 5.0f, curZ);

    // 同步光源可视化球体的变换
    Renderer::Matrix4 lightModel = Renderer::Matrix4::identity();
    lightModel.scaleBy(0.1f, 0.1f, 0.1f);
    lightModel.translate(pImpl->mainLight->position.x, pImpl->mainLight->position.y, pImpl->mainLight->position.z);
    lightModel = lightModel.transpose();
    pImpl->lightSphereRenderable->setTransform(lightModel);

    // 处理拾取（通过 RenderPipeline 的接口）—— 场景点击也通过 GuiLayer 更新选中状态
    if (m_inputState.pickRequested)
    {
        m_inputState.pickRequested = false;

        int sceneX = 0, sceneY = 0;
        bool inScene = m_guiLayer->WindowToSceneViewport(m_inputState.mouseX, m_inputState.mouseY, sceneX, sceneY);

        if (inScene)
        {
            int picked =
                pImpl->renderPipeline->PickObject(static_cast<unsigned int>(sceneX), static_cast<unsigned int>(sceneY));
        if (picked != -1)
        {
                auto renderable = m_scene->GetRenderableByUID(static_cast<unsigned int>(picked));
                m_guiLayer->SetSelectedRenderable(renderable, static_cast<unsigned int>(picked));
        }
        else
        {
                m_guiLayer->SetSelectedRenderable(nullptr, 0);
            }
        }
    }
}

void AppDemo::OnRender(float deltaTime)
{
    // 将渲染管线尺寸同步到 Scene 面板的实际大小，避免宽高比失配导致画面变扁
    int viewportW = m_appConfig.width;
    int viewportH = m_appConfig.height;
    m_guiLayer->GetSceneViewportSize(viewportW, viewportH);
    pImpl->renderPipeline->Resize(viewportW, viewportH);

    // 将 GUI 的 HDR 参数同步到渲染管线
    pImpl->renderPipeline->SetExposure(pImpl->exposure);
    pImpl->renderPipeline->SetTonemapMode(pImpl->tonemapMode);

    // 在编辑器模式下渲染到纹理，交由 Scene 面板显示
    pImpl->renderPipeline->Execute(*m_camera, m_scene->GetRenderables(), *pImpl->mainLight, pImpl->currentSelectedUID,
                                   static_cast<Renderer::ViewMode>(pImpl->gbufferViewMode), pImpl->currentTime, false);
}

void AppDemo::OnGUI()
{
    m_guiLayer->SetSceneViewTexture(pImpl->renderPipeline->GetLastDisplayTexture(),
                                    pImpl->renderPipeline->GetRenderWidth(), pImpl->renderPipeline->GetRenderHeight());

    // 先渲染 GUI（Hierarchy 面板点击会更新 GuiLayer 的选中状态）
    m_guiLayer->RenderGUI();

    // 从 GuiLayer 读回选中状态，保持 AppDemo 与 GUI 同步
    // （场景拾取 和 面板点击 都会更新 GuiLayer，这里统一读取）
    unsigned int guiUID = m_guiLayer->GetSelectedUid();
    pImpl->currentSelectedUID = (guiUID > 0) ? static_cast<int>(guiUID) : -1;
    pImpl->selectedRenderable = (guiUID > 0) ? m_scene->GetRenderableByUID(guiUID) : nullptr;
}

void AppDemo::HandleKeyEvent(int key, int scancode, int action, int mods)
{
    // 调用基类处理
    Application::HandleKeyEvent(key, scancode, action, mods);

    // AppDemo特有的按键处理
    if (key == static_cast<int>(Key::P) && action == ACTION_PRESS)
    {
        m_inputState.togglePoints = !m_inputState.togglePoints;
        LOG_INFO("切换点云渲染模式: {}", m_inputState.togglePoints ? "Points" : "Splats");
    }
}

void AppDemo::SetupScene(std::shared_ptr<::Renderer::CubePrimitive> cubePrimitive,
                         std::shared_ptr<::Renderer::SpherePrimitive> spherePrimitive,
                         std::shared_ptr<::Renderer::QuadPrimitive> quadPrimitive,
                         std::shared_ptr<::Renderer::Material> defaultMaterial,
                         std::shared_ptr<::Renderer::Model> loadedModel,
                         std::shared_ptr<::Renderer::Model> loadedModel2)
{
    // 创建随机球体
    for (int i = 0; i < 30; i++)
    {
        ::Renderer::Matrix4 sphereModel = ::Renderer::Matrix4::identity();
        float radius = ::Renderer::Random::randomFloat(0.1f, 1.0f);
        sphereModel.scaleBy(radius, radius, radius);
        sphereModel.translate(::Renderer::Random::randomFloat(-10.0f, 10.0f),
                              ::Renderer::Random::randomFloat(-10.0f, 10.0f),
                              ::Renderer::Random::randomFloat(-10.0f, 10.0f));
        sphereModel = sphereModel.transpose();

        auto renderable = std::make_shared<::Renderer::Renderable>();
        renderable->setPrimitive(spherePrimitive);
        renderable->setMaterial(defaultMaterial);
        renderable->setTransform(sphereModel);
        renderable->setColor(::Renderer::Random::randomColor());
        m_scene->AddRenderable(renderable);
    }

    // 光源可视化球体（颜色与 Light 一致）
    pImpl->lightSphereRenderable = std::make_shared<::Renderer::Renderable>();
    pImpl->lightSphereRenderable->setPrimitive(spherePrimitive);
    pImpl->lightSphereRenderable->setColor(pImpl->mainLight->color);
    m_scene->AddRenderable(pImpl->lightSphereRenderable);

    // 特效立方体（前向渲染 —— 通过 RenderPipeline 管理）
    ::Renderer::Matrix4 fxSphereModel = ::Renderer::Matrix4::identity();
    fxSphereModel.scaleBy(1.2f, 1.2f, 1.2f);
    fxSphereModel.translate(0.0f, 2.0f, -2.0f);
    fxSphereModel = fxSphereModel.transpose();
    auto fxSphereRenderable = std::make_shared<::Renderer::Renderable>();
    fxSphereRenderable->setPrimitive(cubePrimitive);
    fxSphereRenderable->setMaterial(defaultMaterial);
    fxSphereRenderable->setTransform(fxSphereModel);
    fxSphereRenderable->setColor(::Renderer::Vector3(0.2f, 0.8f, 1.0f));
    pImpl->renderPipeline->AddForwardRenderable(fxSphereRenderable);

    // 地面
    ::Renderer::Matrix4 quadModel = ::Renderer::Matrix4::identity();
    quadModel.scaleBy(10.0f, 10.0f, 10.0f);
    quadModel.rotate(DEG2RAD(-90.0f), ::Renderer::Vector3(1.0f, 0.0f, 0.0f));
    quadModel = quadModel.transpose();
    auto quadRenderable = std::make_shared<::Renderer::Renderable>();
    quadRenderable->setPrimitive(quadPrimitive);
    quadRenderable->setMaterial(defaultMaterial);
    quadRenderable->setTransform(quadModel);
    quadRenderable->setColor(::Renderer::Vector3(0.5f, 0.5f, 0.5f));
    m_scene->AddRenderable(quadRenderable);

    // 模型1
    ::Renderer::Matrix4 model1M = ::Renderer::Matrix4::identity();
    model1M.scaleBy(0.3f, 0.3f, 0.3f);
    model1M.translate(0.0f, 1.0f, 2.0f);
    model1M = model1M.transpose();
    auto model1Renderable = std::make_shared<::Renderer::Renderable>();
    model1Renderable->setModel(loadedModel);
    model1Renderable->setTransform(model1M);
    m_scene->AddRenderable(model1Renderable);

    // 模型2
    ::Renderer::Matrix4 model2M = ::Renderer::Matrix4::identity();
    model2M.scaleBy(0.01f, 0.01f, 0.01f);
    model2M = model2M.transpose();
    auto model2Renderable = std::make_shared<::Renderer::Renderable>();
    model2Renderable->setModel(loadedModel2);
    model2Renderable->setTransform(model2M);
    m_scene->AddRenderable(model2Renderable);
}
