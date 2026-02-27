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
#include "Renderer/PostProcessChain.h"
#include "Renderer/Effects/BloomEffect.h"
#include "Renderer/Renderable.h"
#include "Renderer/ShaderManager.h"
#include <memory>

#if defined(GSENGINE_OS_WINDOWS) || defined(_WIN32)
#include <windows.h>
#include <commdlg.h>
#endif

using namespace GSEngine;

constexpr float DEG_TO_RAD = 3.1415926f / 180.0f;
#define DEG2RAD(x) (x * DEG_TO_RAD)

#if defined(GSENGINE_OS_WINDOWS) || defined(_WIN32)
namespace
{
std::string OpenModelFileDialog()
{
    OPENFILENAMEA ofn = {};
    char buf[1024] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter =
        "Model files (*.glb;*.gltf;*.fbx;*.obj)\0*.glb;*.gltf;*.fbx;*.obj\0All (*.*)\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = sizeof(buf);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameA(&ofn) != 0)
        return std::string(buf);
    return {};
}
}
#else
namespace
{
std::string OpenModelFileDialog()
{
    (void)0;
    return {};
}
}
#endif

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

    std::vector<std::shared_ptr<Renderer::Light>> pointLights;
    std::vector<std::shared_ptr<Renderer::Renderable>> pointLightSphereRenderables;

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
    m_guiLayer->SetGBufferViewModes(&m_renderConfig.viewMode, Renderer::RenderPipeline::GetViewModeLabels());
    m_guiLayer->SetHDRControls(&m_renderConfig.exposure, &m_renderConfig.tonemapMode);
    m_guiLayer->SetSSAOEnabled(&m_renderConfig.ssaoEnabled);
    m_guiLayer->SetScene(m_scene);
    m_guiLayer->SetMaterialManager(m_materialManager);
    m_guiLayer->SetOnLoadModelRequested([this]() { OnLoadModelRequested(); });

    // 加载模型（注入资源管理器）
    AssimpModelLoader modelLoader(*m_textureManager, *m_materialManager);
    std::shared_ptr<Renderer::Model> loadedModel = modelLoader.loadModel(modelPath);
    std::shared_ptr<Renderer::Model> loadedModel2 = modelLoader.loadModel(model2Path);

    // 创建场景光源
    Renderer::Vector3 direction = Renderer::VectorUtils::Normalize(Renderer::Vector3(0.0f, 10.0f, 10.0f));
    pImpl->mainLight = std::make_shared<Renderer::Light>(
        Renderer::Light::CreateDirectionalLight(direction, Renderer::Vector3(1.0f, 1.0f, 1.0f), 1.0f));
    pImpl->mainLight->position = Renderer::Vector3(0.0f, 10.0f, 10.0f);
    pImpl->mainLight->direction = Renderer::VectorUtils::Normalize(-pImpl->mainLight->position);
    m_scene->AddLight(pImpl->mainLight);

    // for (int i = 0; i < 10; i++)
    // {
    //     auto pointLight =
    //         std::make_shared<Renderer::Light>(Renderer::Light::CreatePointLight(Renderer::Vector3(0.0f, 5.0f,
    //         0.0f)));
    //     m_scene->AddLight(pointLight);
    //     pImpl->pointLights.push_back(pointLight);
    // }

    // 通过 ShaderManager 加载前向渲染 Shader
    auto forwardEffectShader = m_shaderManager->LoadShader("forward_effect", "res/shaders/forward_effect.vs.glsl",
                                                           "res/shaders/forward_effect.fs.glsl");
    m_renderPipeline->SetForwardShader(forwardEffectShader);

    // 获取 PostProcessChain 中的 BloomEffect，将参数指针连接到 GUI
    if (auto *ppChain = dynamic_cast<Renderer::PostProcessChain *>(m_renderPipeline->GetPass("PostProcessChain")))
    {
        if (auto *bloom = dynamic_cast<Renderer::BloomEffect *>(ppChain->GetEffect("BloomEffect")))
        {
            m_guiLayer->SetBloomControls(&bloom->threshold, &bloom->intensity, &bloom->blurIterations, &bloom->enabled);
        }
    }

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
    m_renderPipeline.reset();
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
    // pImpl->mainLight->position = Renderer::Vector3(curX, 5.0f, curZ);

    // 同步光源可视化球体的变换
    // pImpl->lightSphereRenderable->m_transform.position = pImpl->mainLight->position;
    pImpl->mainLight->position = pImpl->lightSphereRenderable->m_transform.position;
    // pImpl->mainLight->color = pImpl->lightSphereRenderable->getColor();
    for (int i = 0; i < pImpl->pointLights.size(); i++)
    {
        pImpl->pointLights[i]->position = pImpl->pointLightSphereRenderables[i]->m_transform.position;
        pImpl->pointLights[i]->color = pImpl->pointLightSphereRenderables[i]->getColor();
    }

    // 处理拾取（通过 RenderPipeline 的接口）—— 场景点击也通过 GuiLayer 更新选中状态
    if (m_inputState.pickRequested)
    {
        m_inputState.pickRequested = false;

        int sceneX = 0, sceneY = 0;
        bool inScene = m_guiLayer->WindowToSceneViewport(m_inputState.mouseX, m_inputState.mouseY, sceneX, sceneY);

        if (inScene)
        {
            int picked =
                m_renderPipeline->PickObject(static_cast<unsigned int>(sceneX), static_cast<unsigned int>(sceneY));
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
    m_renderPipeline->Resize(viewportW, viewportH);

    // 将 GUI 的 HDR 参数同步到渲染管线
    m_renderPipeline->SetExposure(m_renderConfig.exposure);
    m_renderPipeline->SetTonemapMode(m_renderConfig.tonemapMode);

    // // 在编辑器模式下渲染到纹理，交由 Scene 面板显示
    // m_renderPipeline->Execute(*m_camera, m_scene->GetRenderables(), m_scene->GetLights(), pImpl->currentSelectedUID,
    //                           static_cast<Renderer::ViewMode>(pImpl->gbufferViewMode), pImpl->currentTime, false);
}

void AppDemo::OnGUI()
{
    m_guiLayer->SetSceneViewTexture(m_renderPipeline->GetLastDisplayTexture(), m_renderPipeline->GetRenderWidth(),
                                    m_renderPipeline->GetRenderHeight());

    // 先渲染 GUI（Hierarchy 面板点击会更新 GuiLayer 的选中状态）
    m_guiLayer->RenderGUI();

    // 从 GuiLayer 读回选中状态，保持 AppDemo 与 GUI 同步
    // （场景拾取 和 面板点击 都会更新 GuiLayer，这里统一读取）
    unsigned int guiUID = m_guiLayer->GetSelectedUid();
    m_renderConfig.selectedUID = (guiUID > 0) ? static_cast<int>(guiUID) : -1;
    pImpl->selectedRenderable = (guiUID > 0) ? m_scene->GetRenderableByUID(guiUID) : nullptr;
}

void AppDemo::OnLoadModelRequested()
{
    std::string path = OpenModelFileDialog();
    if (path.empty())
        return;
    AssimpModelLoader loader(*m_textureManager, *m_materialManager);
    std::shared_ptr<Renderer::Model> model = loader.loadModel(path);
    if (!model)
    {
        LOG_ERROR("Failed to load model: {}", path);
        return;
    }
    auto renderable = std::make_shared<Renderer::Renderable>();
    renderable->setModel(model);
    renderable->m_transform.position = Renderer::Vector3(0.0f, 1.0f, 0.0f);
    m_scene->AddRenderable(renderable);
    LOG_INFO("Loaded model: {}", path);
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
        float radius = ::Renderer::Random::randomFloat(0.1f, 1.0f);

        ::Renderer::Vector3 scale = ::Renderer::Vector3(radius, radius, radius);
        ::Renderer::Vector3 position = ::Renderer::Vector3(::Renderer::Random::randomFloat(-10.0f, 10.0f),
                                                           ::Renderer::Random::randomFloat(-10.0f, 10.0f),
                                                           ::Renderer::Random::randomFloat(-10.0f, 10.0f));

        auto renderable = std::make_shared<::Renderer::Renderable>();
        renderable->setPrimitive(spherePrimitive);
        renderable->setMaterial(defaultMaterial);
        renderable->m_transform.position = position;
        renderable->m_transform.scale = scale;
        renderable->setColor(::Renderer::Random::randomColor());
        m_scene->AddRenderable(renderable);
    }

    // 光源可视化球体（颜色与 Light 一致）
    // auto emissiveShader =
    // m_shaderManager->LoadShader("emissive", "res/shaders/basepass.vs.glsl", "res/shaders/emissive.fs.glsl");
    pImpl->lightSphereRenderable = std::make_shared<::Renderer::Renderable>();
    pImpl->lightSphereRenderable->setPrimitive(spherePrimitive);
    pImpl->lightSphereRenderable->setColor(pImpl->mainLight->color * 50.0f);
    pImpl->lightSphereRenderable->m_transform.scale = Renderer::Vector3(0.2f, 0.2f, 0.2f);
    pImpl->lightSphereRenderable->m_transform.position = pImpl->mainLight->position;
    pImpl->lightSphereRenderable->setName("Main Light");
    m_scene->AddRenderable(pImpl->lightSphereRenderable);

    for (int i = 0; i < pImpl->pointLights.size(); i++)
    {
        pImpl->pointLights[i]->position = ::Renderer::Random::randomVector3(-3.0f, 10.0f);
        pImpl->pointLights[i]->color = ::Renderer::Random::randomColor();
        auto lightSphereRenderable = std::make_shared<::Renderer::Renderable>();
        lightSphereRenderable->setPrimitive(spherePrimitive);
        lightSphereRenderable->setColor(pImpl->pointLights[i]->color);
        float scale = ::Renderer::Random::randomFloat(0.1f, 0.3f);
        lightSphereRenderable->m_transform.scale = Renderer::Vector3(scale, scale, scale);
        lightSphereRenderable->m_transform.position = pImpl->pointLights[i]->position;
        lightSphereRenderable->setName("Point Light " + std::to_string(i));
        m_scene->AddRenderable(lightSphereRenderable);
        pImpl->pointLightSphereRenderables.push_back(lightSphereRenderable);
    }

    // 特效立方体（前向渲染 —— 通过 RenderPipeline 管理）
    auto fxSphereRenderable = std::make_shared<::Renderer::Renderable>();
    fxSphereRenderable->setPrimitive(cubePrimitive);
    fxSphereRenderable->setMaterial(defaultMaterial);
    fxSphereRenderable->m_transform.position = ::Renderer::Vector3(0.0f, 2.0f, -2.0f);
    fxSphereRenderable->m_transform.scale = ::Renderer::Vector3(1.2f, 1.2f, 1.2f);
    fxSphereRenderable->setColor(::Renderer::Vector3(0.2f, 0.8f, 1.0f));
    m_renderPipeline->AddForwardRenderable(fxSphereRenderable);

    // 地面
    auto quadRenderable = std::make_shared<::Renderer::Renderable>();
    quadRenderable->setPrimitive(quadPrimitive);
    quadRenderable->setMaterial(defaultMaterial);
    quadRenderable->m_transform.position = ::Renderer::Vector3(0.0f, 0.0f, 0.0f);
    quadRenderable->m_transform.scale = ::Renderer::Vector3(10.0f, 10.0f, 10.0f);
    quadRenderable->m_transform.rotation = ::Renderer::Rotator(-90.0f, 0.0f, 0.0f);
    quadRenderable->setColor(::Renderer::Vector3(0.5f, 0.5f, 0.5f));
    m_scene->AddRenderable(quadRenderable);

    // 模型1
    auto model1Renderable = std::make_shared<::Renderer::Renderable>();
    model1Renderable->setModel(loadedModel);
    model1Renderable->m_transform.position = ::Renderer::Vector3(0.0f, 1.0f, 2.0f);
    model1Renderable->m_transform.scale = ::Renderer::Vector3(0.3f, 0.3f, 0.3f);
    m_scene->AddRenderable(model1Renderable);

    // 模型2
    auto model2Renderable = std::make_shared<::Renderer::Renderable>();
    model2Renderable->setModel(loadedModel2);
    model2Renderable->m_transform.position = ::Renderer::Vector3(0.0f, 0.0f, 0.0f);
    model2Renderable->m_transform.scale = ::Renderer::Vector3(0.01f, 0.01f, 0.01f);
    m_scene->AddRenderable(model2Renderable);
}
