#include "AppDemo.h"
#include "Logger/Log.h"
#include "Renderer/FinalPass.h"
#include "Renderer/ForwardPass.h"
#include "Renderer/GeometryPass.h"
#include "Renderer/LightingPass.h"
#include "Renderer/MathUtils/Random.h"
#include "Renderer/PostProcessPass.h"
#include "Renderer/Primitives/CubePrimitive.h"
#include "Renderer/Primitives/QuadPrimitive.h"
#include "Renderer/Primitives/SpherePrimitive.h"
#include "Renderer/Renderable.h"
#include "Renderer/Shader.h"
#include "ModelLoader/AssimpModelLoader.h"
#include "Assets/MaterialManager.h"
#include <memory>

GSENGINE_NAMESPACE_BEGIN

constexpr float DEG_TO_RAD = 3.1415926f / 180.0f;
#define DEG2RAD(x) (x * DEG_TO_RAD)

#ifdef RENDERER_DEBUG
std::string modelPath = "./res/backpack/backpack.obj";
std::string model2Path = "./res/houtou.fbx";
#endif

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

// AppDemo的私有成员变量
class AppDemo::Impl
{
public:
    // 渲染管线
    std::unique_ptr<Renderer::GeometryPass> geometryPass;
    std::unique_ptr<Renderer::LightingPass> lightingPass;
    std::unique_ptr<Renderer::PostProcessPass> postProcessPass;
    std::unique_ptr<Renderer::ForwardPass> forwardPass;
    std::unique_ptr<Renderer::FinalPass> finalPass;

    // 渲染资源
    std::shared_ptr<Renderer::Shader> forwardEffectShader;
    std::vector<std::shared_ptr<Renderer::Renderable>> forwardRenderables;
    std::shared_ptr<Renderer::Renderable> lightSphereRenderable;

    // GUI状态
    int gbufferViewMode = static_cast<int>(ViewMode::Final);
    std::vector<const char*> gbufferViewLabels = {
        "Final (PostProcess)", "Lighting", "Position", "Normal", "Diffuse", "Specular", "Shininess", "Depth"};

    // 应用状态
    unsigned int currentSelectedUID = 0;
    std::shared_ptr<Renderer::Renderable> selectedRenderable = nullptr;
    float currentTime = 0.0f;

    // 矩阵
    float viewMatrix[16];
    float projMatrix[16];
};

AppDemo::AppDemo(AppConfig config) : Application(config), pImpl(std::make_unique<Impl>())
{
}

AppDemo::~AppDemo() = default;

bool AppDemo::OnInit()
{
    LOG_INFO("AppDemo 初始化中...");

    // 设置GUI
    m_guiLayer->SetGBufferViewModes(&pImpl->gbufferViewMode, pImpl->gbufferViewLabels);
    m_guiLayer->SetScene(m_scene);

    // 加载模型
    AssimpModelLoader modelLoader;
    std::shared_ptr<Renderer::Model> loadedModel = modelLoader.loadModel(modelPath);
    std::shared_ptr<Renderer::Model> loadedModel2 = modelLoader.loadModel(model2Path);

    // 初始化渲染管线
    pImpl->geometryPass = std::make_unique<Renderer::GeometryPass>(m_appConfig.width, m_appConfig.height);
    pImpl->lightingPass = std::make_unique<Renderer::LightingPass>(m_appConfig.width, m_appConfig.height);
    pImpl->postProcessPass = std::make_unique<Renderer::PostProcessPass>(m_appConfig.width, m_appConfig.height);
    pImpl->forwardPass = std::make_unique<Renderer::ForwardPass>();
    pImpl->finalPass = std::make_unique<Renderer::FinalPass>();

    // 初始化渲染资源
    pImpl->forwardEffectShader = Renderer::Shader::fromFiles("res/shaders/forward_effect.vs.glsl", "res/shaders/forward_effect.fs.glsl");

    // 创建几何体
    auto cubePrimitive = std::make_shared<Renderer::CubePrimitive>(1.0f);
    auto spherePrimitive = std::make_shared<Renderer::SpherePrimitive>(1.0f, 64, 32);
    auto quadPrimitive = std::make_shared<Renderer::QuadPrimitive>(10.0f);
    auto defaultMaterial = MaterialManager::GetInstance()->GetDefaultMaterial();

    // 创建场景内容
    SetupScene(cubePrimitive, spherePrimitive, quadPrimitive, defaultMaterial, loadedModel, loadedModel2);

    LOG_INFO("AppDemo 初始化完成");
    return true;
}

void AppDemo::OnUpdate(float deltaTime)
{
    pImpl->currentTime += deltaTime;

    // 更新光源位置
    Renderer::Vector3 lightPos(0.0f, 5.0f, 0.0f);
    float curX = 5.0f * std::sin(pImpl->currentTime);
    float curZ = 5.0f * std::cos(pImpl->currentTime);
    lightPos.x = curX;
    lightPos.z = curZ;

    // 更新光源球体变换
    Renderer::Matrix4 lightModel = Renderer::Matrix4::identity();
    lightModel.scaleBy(0.1f, 0.1f, 0.1f);
    lightModel.translate(lightPos.x, lightPos.y, lightPos.z);
    lightModel = lightModel.transpose();
    pImpl->lightSphereRenderable->setTransform(lightModel);

    // 处理拾取
    if (m_inputState.pickRequested)
    {
        unsigned int mouseXInt = static_cast<unsigned int>(m_inputState.mouseX);
        unsigned int mouseYInt = m_appConfig.height - static_cast<unsigned int>(m_inputState.mouseY);

        unsigned int picked = pImpl->geometryPass->getCurrentSelectedUID(mouseXInt, mouseYInt);
        m_inputState.pickRequested = false;

        if (picked != 0)
        {
            pImpl->currentSelectedUID = picked;
            pImpl->selectedRenderable = m_scene->GetRenderableByUID(picked);
        }
    }
}

void AppDemo::OnRender(float deltaTime)
{
    // 更新视图和投影矩阵
    m_camera->getViewMatrix(pImpl->viewMatrix);
    m_camera->getPerspectiveMatrix(pImpl->projMatrix, 45.0f,
                                   static_cast<float>(m_appConfig.width) / static_cast<float>(m_appConfig.height),
                                   0.01f, 1000.0f);

    // 设置forward shader的uniform
    float vx, vy, vz;
    m_camera->getPosition(vx, vy, vz);
    pImpl->forwardEffectShader->use();
    pImpl->forwardEffectShader->setVec3("u_viewPos", vx, vy, vz);
    pImpl->forwardEffectShader->unuse();

    // 几何通道
    pImpl->geometryPass->Begin(pImpl->viewMatrix, pImpl->projMatrix);
    for (auto &renderable : m_scene->GetRenderables())
    {
        if (renderable)
            pImpl->geometryPass->Render(renderable.get());
    }
    pImpl->geometryPass->End();

    // 计算光源位置
    ::Renderer::Vector3 lightPos(0.0f, 5.0f, 0.0f);
    float curX = 5.0f * std::sin(pImpl->currentTime);
    float curZ = 5.0f * std::cos(pImpl->currentTime);
    lightPos.x = curX;
    lightPos.z = curZ;

    // 光照通道
    pImpl->lightingPass->Begin(*m_camera, lightPos);
    pImpl->lightingPass->Render(
        pImpl->geometryPass->getPositionTexture(),
        pImpl->geometryPass->getNormalTexture(),
        pImpl->geometryPass->getDiffuseTexture(),
        pImpl->geometryPass->getSpecularTexture(),
        pImpl->geometryPass->getShininessTexture()
    );
    pImpl->lightingPass->End();

    // 后处理通道
    pImpl->postProcessPass->render(
        m_appConfig.width, m_appConfig.height, *m_camera, pImpl->currentSelectedUID,
        pImpl->geometryPass->getUIDTexture(),
        pImpl->geometryPass->getPositionTexture(),
        pImpl->geometryPass->getNormalTexture(),
        pImpl->lightingPass->getLightingTexture(),
        pImpl->geometryPass->getDepthTexture()
    );

    // 正向渲染（特效物体）
    pImpl->forwardPass->Render(
        m_appConfig.width, m_appConfig.height,
        pImpl->viewMatrix, pImpl->projMatrix,
        pImpl->postProcessPass->getColorTexture(),
        pImpl->geometryPass->getDepthTexture(),
        pImpl->forwardRenderables,
        pImpl->forwardEffectShader,
        pImpl->currentTime
    );

    // 选择显示的纹理
    unsigned int displayTex = pImpl->postProcessPass->getColorTexture();
    switch (static_cast<ViewMode>(pImpl->gbufferViewMode))
    {
    case ViewMode::Final:
        displayTex = pImpl->postProcessPass->getColorTexture();
        break;
    case ViewMode::Lighting:
        displayTex = pImpl->lightingPass->getLightingTexture();
        break;
    case ViewMode::Position:
        displayTex = pImpl->geometryPass->getPositionTexture();
        break;
    case ViewMode::Normal:
        displayTex = pImpl->geometryPass->getNormalTexture();
        break;
    case ViewMode::Diffuse:
        displayTex = pImpl->geometryPass->getDiffuseTexture();
        break;
    case ViewMode::Specular:
        displayTex = pImpl->geometryPass->getSpecularTexture();
        break;
    case ViewMode::Shininess:
        displayTex = pImpl->geometryPass->getShininessTexture();
        break;
    case ViewMode::Depth:
        displayTex = pImpl->geometryPass->getDepthTexture();
        break;
    }

    // 最终通道
    pImpl->finalPass->render(m_appConfig.width, m_appConfig.height, displayTex);
}

void AppDemo::OnGUI()
{
    if (pImpl->selectedRenderable)
    {
        m_guiLayer->SetSelectedRenderable(pImpl->selectedRenderable, pImpl->currentSelectedUID);
    }
    m_guiLayer->RenderGUI();
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

void AppDemo::SetupScene(
    std::shared_ptr<::Renderer::CubePrimitive> cubePrimitive,
    std::shared_ptr<::Renderer::SpherePrimitive> spherePrimitive,
    std::shared_ptr<::Renderer::QuadPrimitive> quadPrimitive,
    std::shared_ptr<::Renderer::Material> defaultMaterial,
    std::shared_ptr<::Renderer::Model> loadedModel,
    std::shared_ptr<::Renderer::Model> loadedModel2
)
{
    // 创建随机球体
    for (int i = 0; i < 30; i++)
    {
        ::Renderer::Matrix4 sphereModel = ::Renderer::Matrix4::identity();
        float radius = ::Renderer::Random::randomFloat(0.1f, 1.0f);
        sphereModel.scaleBy(radius, radius, radius);
        sphereModel.translate(
            ::Renderer::Random::randomFloat(-10.0f, 10.0f),
            ::Renderer::Random::randomFloat(-10.0f, 10.0f),
            ::Renderer::Random::randomFloat(-10.0f, 10.0f)
        );
        sphereModel = ::Renderer::Matrix4::transpose(sphereModel);

        auto renderable = std::make_shared<::Renderer::Renderable>();
        renderable->setPrimitive(spherePrimitive);
        renderable->setMaterial(defaultMaterial);
        renderable->setTransform(sphereModel);
        renderable->setColor(::Renderer::Random::randomColor());
        m_scene->AddRenderable(renderable);
    }

    // 光源球体
    pImpl->lightSphereRenderable = std::make_shared<::Renderer::Renderable>();
    pImpl->lightSphereRenderable->setPrimitive(spherePrimitive);
    pImpl->lightSphereRenderable->setColor(::Renderer::Vector3(1.0f, 1.0f, 1.0f));
    m_scene->AddRenderable(pImpl->lightSphereRenderable);

    // 特效立方体（正向渲染）
    ::Renderer::Matrix4 fxSphereModel = ::Renderer::Matrix4::identity();
    fxSphereModel.scaleBy(1.2f, 1.2f, 1.2f);
    fxSphereModel.translate(0.0f, 2.0f, -2.0f);
    fxSphereModel = ::Renderer::Matrix4::transpose(fxSphereModel);
    auto fxSphereRenderable = std::make_shared<::Renderer::Renderable>();
    fxSphereRenderable->setPrimitive(cubePrimitive);
    fxSphereRenderable->setMaterial(defaultMaterial);
    fxSphereRenderable->setTransform(fxSphereModel);
    fxSphereRenderable->setColor(::Renderer::Vector3(0.2f, 0.8f, 1.0f));
    pImpl->forwardRenderables.push_back(fxSphereRenderable);

    // 地面
    ::Renderer::Matrix4 quadModel = ::Renderer::Matrix4::identity();
    quadModel.scaleBy(10.0f, 10.0f, 10.0f);
    quadModel.rotate(DEG2RAD(-90.0f), ::Renderer::Vector3(1.0f, 0.0f, 0.0f));
    quadModel = ::Renderer::Matrix4::transpose(quadModel);
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
    model1M = ::Renderer::Matrix4::transpose(model1M);
    auto model1Renderable = std::make_shared<::Renderer::Renderable>();
    model1Renderable->setModel(loadedModel);
    model1Renderable->setTransform(model1M);
    m_scene->AddRenderable(model1Renderable);

    // 模型2
    ::Renderer::Matrix4 model2M = ::Renderer::Matrix4::identity();
    model2M.scaleBy(0.01f, 0.01f, 0.01f);
    model2M = ::Renderer::Matrix4::transpose(model2M);
    auto model2Renderable = std::make_shared<::Renderer::Renderable>();
    model2Renderable->setModel(loadedModel2);
    model2Renderable->setTransform(model2M);
    m_scene->AddRenderable(model2Renderable);
}
GSENGINE_NAMESPACE_END

