#include "Application.h"
#include "Event/EventBus.h"
#include "Gui/GuiLayer.h"
#include "Logger/Log.h"
#include "Renderer/Camera.h"
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
#include "Scene/Scene.h"
#include "Window/Window.h"
#include "ModelLoader/AssimpModelLoader.h"
#include "Assets/MaterialManager.h"

GSENGINE_NAMESPACE_BEGIN

struct InputState
{
    bool moveForward{false};
    bool moveBackward{false};
    bool moveLeft{false};
    bool moveRight{false};
    bool moveUp{false};
    bool moveDown{false};
    bool leftMouseDown{false};
    bool rightMouseDown{false};
    bool pickRequested{false};
    bool togglePoints{false};
    bool exitRequested{false};
    bool firstMouse{true};
    double lastX{0.0};
    double lastY{0.0};
    double mouseX{0.0};
    double mouseY{0.0};
};

constexpr int ACTION_RELEASE = 0;
constexpr int ACTION_PRESS = 1;

constexpr float DEG_TO_RAD = 3.1415926f / 180.0f;
#define DEG2RAD(x) (x * DEG_TO_RAD)

#ifdef RENDERER_DEBUG
std::string modelPath = "./res/backpack/backpack.obj";
std::string model2Path = "./res/houtou.fbx";
// std::string model2Path = "./res/monkey.glb";
#else
#endif

static int WIN_WIDTH = 1920;
static int WIN_HEIGHT = 1080;

EventBus eventBus;
GuiLayer guiLayer;
InputState inputState;
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
static int gbufferViewMode = static_cast<int>(ViewMode::Final);
std::vector<const char *> gbufferViewLabels = {
    "Final (PostProcess)", "Lighting", "Position", "Normal", "Diffuse", "Specular", "Shininess", "Depth"};

Renderer::FinalPass *finalPassPtr = nullptr;
Renderer::GeometryPass *geometryPassPtr = nullptr;
Renderer::LightingPass *lightingPassPtr = nullptr;
Renderer::PostProcessPass *postProcessPassPtr = nullptr;
Renderer::ForwardPass *forwardPassPtr = nullptr;
std::shared_ptr<Renderer::Shader> forwardEffectShaderPtr = nullptr;
std::shared_ptr<Scene> scene = nullptr;
std::vector<std::shared_ptr<Renderer::Renderable>> forwardRenderables;
std::shared_ptr<Renderer::Renderable> lightSphereRenderable = nullptr;

float lastTime = 0.0f;
int frameCount = 0;
float startTime = 0.0f;
unsigned int currentSelectedUID = 0;
std::shared_ptr<Renderer::Renderable> selectedRenderable = nullptr;
static const Renderer::Vector3 camPos(4.42f, 1.0f, -3.63f);
Renderer::Camera camera(camPos,                              // 位置（模型中心前方）
                            Renderer::Vector3(0.0f, 1.0f, 0.0f), // 世界上方向
                            133.5f,                              // yaw: -90度 朝向 -Z 方向（看向模型）
                            -14.0f                               // pitch: 0度 水平视角
    );
float viewMatrix[16];
float projMatrix[16];
Application::Application(AppConfig config) : m_appConfig(config)
{
    WIN_WIDTH = m_appConfig.width;
    WIN_HEIGHT = m_appConfig.height;
}

Application::~Application()
{
}

bool Application::Init()
{
    WIN_WIDTH = m_appConfig.width;
    WIN_HEIGHT = m_appConfig.height;

    // 初始化日志系统
    Logger::Log::Init();

    LOG_INFO("3DGS Engine 启动中...");

    // 初始化 GLFW
    if (!Window::initGLFW())
    {
        LOG_ERROR("初始化 GLFW 失败");
        return false;
    }
    LOG_INFO("GLFW 初始化成功");

    // 创建窗口（自动初始化 OpenGL 上下文）
    m_window = new Window(WIN_WIDTH, WIN_HEIGHT, "3DGS Renderer");
    Window &window = *m_window;
    LOG_INFO("窗口创建成功: {}x{}", WIN_WIDTH, WIN_HEIGHT);

    // 先注册回调，后续 ImGui 安装的回调会自动进行链式转发
    window.setKeyCallback([](int key, int scancode, int action, int mods) {
        eventBus.Emplace<KeyEvent>(key, scancode, action, mods, Window::getTime());
    });
    window.setMouseButtonCallback([](int button, int action, int mods, double xpos, double ypos) {
        inputState.mouseX = xpos;
        inputState.mouseY = ypos;
        eventBus.Emplace<MouseButtonEvent>(button, action, mods, xpos, ypos, Window::getTime());
    });
    window.setMouseMoveCallback([](double xpos, double ypos) {
        double dx = 0.0;
        double dy = 0.0;
        if (inputState.firstMouse)
        {
            inputState.lastX = xpos;
            inputState.lastY = ypos;
            inputState.firstMouse = false;
        }
        else
        {
            dx = xpos - inputState.lastX;
            dy = inputState.lastY - ypos;
            if (std::abs(dx) > 100.0 || std::abs(dy) > 100.0)
            {
                inputState.lastX = xpos;
                inputState.lastY = ypos;
                return;
            }
            inputState.lastX = xpos;
            inputState.lastY = ypos;
        }
        inputState.mouseX = xpos;
        inputState.mouseY = ypos;
        eventBus.Emplace<MouseMoveEvent>(xpos, ypos, dx, dy, Window::getTime());
    });
    window.setMouseScrollCallback(
        [](double xoffset, double yoffset) { eventBus.Emplace<ScrollEvent>(xoffset, yoffset, Window::getTime()); });

    guiLayer.Init(&window);

    guiLayer.SetGBufferViewModes(&gbufferViewMode, gbufferViewLabels);

    AssimpModelLoader modelLoader;
    std::shared_ptr<Renderer::Model> loadedModel = modelLoader.loadModel(modelPath);
    std::shared_ptr<Renderer::Model> loadedModel2 = modelLoader.loadModel(model2Path);

    camera.setMovementSpeed(2.0f); // 增大移动速度，因为场景较大
    camera.setMouseSensitivity(0.1f);

    float x, y, z;
    camera.getPosition(x, y, z);
    LOG_INFO("相机创建成功，位置: ({}, {}, {})", x, y, z);

    eventBus.Subscribe(EventType::Key, 10, [&](Event &evt) {
        auto &e = static_cast<KeyEvent &>(evt);
        if (guiLayer.WantCaptureKeyboard())
            return;
        bool pressed = e.action != ACTION_RELEASE;
        if (e.key == static_cast<int>(Key::W))
            inputState.moveForward = pressed;
        if (e.key == static_cast<int>(Key::S))
            inputState.moveBackward = pressed;
        if (e.key == static_cast<int>(Key::A))
            inputState.moveLeft = pressed;
        if (e.key == static_cast<int>(Key::D))
            inputState.moveRight = pressed;
        if (e.key == static_cast<int>(Key::Q))
            inputState.moveDown = pressed;
        if (e.key == static_cast<int>(Key::E))
            inputState.moveUp = pressed;
        if (e.key == static_cast<int>(Key::Escape) && pressed)
            inputState.exitRequested = true;
        if (e.key == static_cast<int>(Key::P) && e.action == ACTION_PRESS)
        {
            inputState.togglePoints = !inputState.togglePoints;
            LOG_INFO("切换点云渲染模式: {}", inputState.togglePoints ? "Points" : "Splats");
        }
    });

    eventBus.Subscribe(EventType::MouseButton, 10, [&](Event &evt) {
        auto &e = static_cast<MouseButtonEvent &>(evt);
        if (guiLayer.WantCaptureMouse())
            return;
        if (e.button == static_cast<int>(MouseButton::Left))
        {
            inputState.leftMouseDown = e.action != ACTION_RELEASE;
            if (e.action == ACTION_PRESS)
                inputState.pickRequested = true;
        }
        if (e.button == static_cast<int>(MouseButton::Right))
        {
            inputState.rightMouseDown = e.action != ACTION_RELEASE;
            inputState.firstMouse = (e.action == ACTION_RELEASE);
            inputState.lastX = e.x;
            inputState.lastY = e.y;
            LOG_INFO("Right mouse button pressed at ({}, {})", e.x, e.y);
            LOG_INFO("Right mouse button down: {}", inputState.rightMouseDown);
        }
    });

    eventBus.Subscribe(EventType::MouseMove, 10, [&](Event &evt) {
        auto &e = static_cast<MouseMoveEvent &>(evt);
        if (guiLayer.WantCaptureMouse())
            return;
        inputState.mouseX = e.x;
        inputState.mouseY = e.y;
        if (!inputState.rightMouseDown)
            return;
        camera.processMouseMovement(static_cast<float>(e.dx), static_cast<float>(e.dy));
        LOG_INFO("Mouse moved to ({}, {})", e.x, e.y);
        LOG_INFO("Mouse moved dx: {}, dy: {}", e.dx, e.dy);
    });

    eventBus.Subscribe(EventType::Scroll, 10, [&](Event &evt) {
        auto &e = static_cast<ScrollEvent &>(evt);
        if (guiLayer.WantCaptureMouse())
            return;
        camera.processMouseScroll(static_cast<float>(e.yoffset));
    });

    // 先设置光标模式为禁用（FPS模式），这会锁定并隐藏鼠标
    // window.setCursorMode(Renderer::Window::CursorMode::Disabled);

    // 创建彩色立方体
    std::shared_ptr<Renderer::CubePrimitive> cubePrimitive = std::make_shared<Renderer::CubePrimitive>(1.0f);

    std::shared_ptr<Renderer::SpherePrimitive> spherePrimitive = std::make_shared<Renderer::SpherePrimitive>(1.0f, 64, 32);

    std::shared_ptr<Renderer::QuadPrimitive> quadPrimitive = std::make_shared<Renderer::QuadPrimitive>(10.0f);

    // 测试相机矩阵
    camera.getViewMatrix(viewMatrix);
    camera.getPerspectiveMatrix(projMatrix, 50.0f, static_cast<float>(WIN_WIDTH) / static_cast<float>(WIN_HEIGHT),
                                0.01f, 1000.0f);
    geometryPassPtr = new Renderer::GeometryPass(WIN_WIDTH, WIN_HEIGHT);
    lightingPassPtr = new Renderer::LightingPass(WIN_WIDTH, WIN_HEIGHT);
    postProcessPassPtr = new Renderer::PostProcessPass(WIN_WIDTH, WIN_HEIGHT);
    forwardPassPtr = new Renderer::ForwardPass();
    finalPassPtr = new Renderer::FinalPass();
    scene = std::make_shared<Scene>();
    lightSphereRenderable = std::make_shared<Renderer::Renderable>();
    std::shared_ptr<Renderer::Material> defaultMaterial = MaterialManager::GetInstance()->GetDefaultMaterial();

    guiLayer.SetScene(scene);
    forwardEffectShaderPtr = Renderer::Shader::fromFiles("res/shaders/forward_effect.vs.glsl", "res/shaders/forward_effect.fs.glsl");

    float minX = -10.0f;
    float maxX = 10.0f;
    float minY = -10.0f;
    float maxY = 10.0f;
    float minZ = -10.0f;
    float maxZ = 10.0f;
    float minRadius = 0.1f;
    float maxRadius = 1.0f;
    for (int i = 0; i < 30; i++)
    {
        Renderer::Matrix4 sphereModel = Renderer::Matrix4::identity();
        float radius = Renderer::Random::randomFloat(minRadius, maxRadius);
        sphereModel.scaleBy(radius, radius, radius);
        sphereModel.translate(Renderer::Random::randomFloat(minX, maxX), Renderer::Random::randomFloat(minY, maxY),
                              Renderer::Random::randomFloat(minZ, maxZ));
        sphereModel = sphereModel.transpose();
        auto renderable = std::make_shared<Renderer::Renderable>();
        renderable->setPrimitive(spherePrimitive);
        renderable->setMaterial(defaultMaterial);
        renderable->setTransform(sphereModel);
        renderable->setColor(Renderer::Random::randomColor());
        scene->AddRenderable(renderable);
    }
    // 轻量光源球体
    lightSphereRenderable->setPrimitive(spherePrimitive);
    lightSphereRenderable->setColor(Renderer::Vector3(1.0f, 1.0f, 1.0f));
    scene->AddRenderable(lightSphereRenderable);
    // 特效半透明球体示例（正向渲染，不进入延迟管线）
    Renderer::Matrix4 fxSphereModel = Renderer::Matrix4::identity();
    fxSphereModel.scaleBy(1.2f, 1.2f, 1.2f);
    fxSphereModel.translate(0.0f, 2.0f, -2.0f);
    fxSphereModel = fxSphereModel.transpose();
    auto fxSphereRenderable = std::make_shared<Renderer::Renderable>();
    fxSphereRenderable->setPrimitive(cubePrimitive);
    fxSphereRenderable->setMaterial(defaultMaterial);
    fxSphereRenderable->setTransform(fxSphereModel);
    fxSphereRenderable->setColor(Renderer::Vector3(0.2f, 0.8f, 1.0f));
    forwardRenderables.push_back(fxSphereRenderable);
    // 地面
    Renderer::Matrix4 quadModel = Renderer::Matrix4::identity();
    quadModel.scaleBy(10.0f, 10.0f, 10.0f);
    quadModel.rotate(DEG2RAD(-90.0f), Renderer::Vector3(1.0f, 0.0f, 0.0f));
    quadModel = quadModel.transpose();
    auto quadRenderable = std::make_shared<Renderer::Renderable>();
    quadRenderable->setPrimitive(quadPrimitive);
    quadRenderable->setMaterial(defaultMaterial);
    quadRenderable->setTransform(quadModel);
    quadRenderable->setColor(Renderer::Vector3(0.5f, 0.5f, 0.5f));
    scene->AddRenderable(quadRenderable);
    // 模型实例
    Renderer::Matrix4 model1M = Renderer::Matrix4::identity();
    model1M.scaleBy(0.3f, 0.3f, 0.3f);
    model1M.translate(0.0f, 1.0f, 2.0f);
    model1M = model1M.transpose();
    auto model1Renderable = std::make_shared<Renderer::Renderable>();
    model1Renderable->setModel(loadedModel);
    model1Renderable->setTransform(model1M);
    scene->AddRenderable(model1Renderable);

    Renderer::Matrix4 model2M = Renderer::Matrix4::identity();
    model2M.scaleBy(0.01f, 0.01f, 0.01f);
    model2M = model2M.transpose();
    auto model2Renderable = std::make_shared<Renderer::Renderable>();
    model2Renderable->setModel(loadedModel2);
    model2Renderable->setTransform(model2M);
    scene->AddRenderable(model2Renderable);

    startTime = Window::getTime();

    LOG_INFO("Application Init Success!");
    return true;
}

void Application::Run()
{
    while (!m_window->shouldClose())
    {
        float currentTime = static_cast<float>(m_window->getTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        frameCount++;
        if (currentTime - startTime >= 1.0f)
        {
            float fps = static_cast<float>(frameCount) / (currentTime - startTime);
            LOG_INFO("FPS: {}", int(fps));
            startTime = currentTime;
            frameCount = 0;
        }
        m_window->pollEvents();
        guiLayer.BeginFrame();
        eventBus.Dispatch();

        if (inputState.exitRequested)
            break;

        if (inputState.moveForward)
            camera.processKeyboard(Renderer::CameraMovement::Forward, deltaTime);
        if (inputState.moveBackward)
            camera.processKeyboard(Renderer::CameraMovement::Backward, deltaTime);
        if (inputState.moveLeft)
            camera.processKeyboard(Renderer::CameraMovement::Left, deltaTime);
        if (inputState.moveRight)
            camera.processKeyboard(Renderer::CameraMovement::Right, deltaTime);
        if (inputState.moveUp)
            camera.processKeyboard(Renderer::CameraMovement::Up, deltaTime);
        if (inputState.moveDown)
            camera.processKeyboard(Renderer::CameraMovement::Down, deltaTime);

        unsigned int mouseXInt = static_cast<unsigned int>(inputState.mouseX);
        unsigned int mouseYInt = WIN_HEIGHT - static_cast<unsigned int>(inputState.mouseY);

        Renderer::Vector3 lightPos(0.0f, 5.0f, 0.0f);
        float curX = 5.0f * std::sin(currentTime);
        float curZ = 5.0f * std::cos(currentTime);
        lightPos.x = curX;
        lightPos.z = curZ;
        // 更新光源球体变换
        Renderer::Matrix4 lightModel = Renderer::Matrix4::identity();
        lightModel.scaleBy(0.1f, 0.1f, 0.1f);
        lightModel.translate(lightPos.x, lightPos.y, lightPos.z);
        lightModel = lightModel.transpose();
        lightSphereRenderable->setTransform(lightModel);

        // 更新视图矩阵
        camera.getViewMatrix(viewMatrix);
        float vx, vy, vz;
        camera.getPosition(vx, vy, vz);
        forwardEffectShaderPtr->use();
        forwardEffectShaderPtr->setVec3("u_viewPos", vx, vy, vz);
        forwardEffectShaderPtr->unuse();
        geometryPassPtr->Begin(viewMatrix, projMatrix);
        for (auto &r : scene->GetRenderables())
        {
            if (!r)
                continue;
            geometryPassPtr->Render(r.get());
        }
        geometryPassPtr->End();
        if (inputState.pickRequested)
        {
            unsigned int picked = geometryPassPtr->getCurrentSelectedUID(mouseXInt, mouseYInt);
            inputState.pickRequested = false;
            if (picked != 0)
            {
                currentSelectedUID = picked;
                selectedRenderable = scene->GetRenderableByUID(picked);
            }
            currentSelectedUID = picked;
        }

        lightingPassPtr->Begin(camera, lightPos);
        lightingPassPtr->Render(geometryPassPtr->getPositionTexture(), geometryPassPtr->getNormalTexture(),
                                geometryPassPtr->getDiffuseTexture(), geometryPassPtr->getSpecularTexture(),
                                geometryPassPtr->getShininessTexture());
        lightingPassPtr->End();

        postProcessPassPtr->render(WIN_WIDTH, WIN_HEIGHT, camera, currentSelectedUID, geometryPassPtr->getUIDTexture(),
                                   geometryPassPtr->getPositionTexture(), geometryPassPtr->getNormalTexture(),
                                   lightingPassPtr->getLightingTexture(), geometryPassPtr->getDepthTexture());
        // 正向渲染队列（半透明/特效物体）叠加到后处理颜色缓冲
        forwardPassPtr->Render(WIN_WIDTH, WIN_HEIGHT, viewMatrix, projMatrix, postProcessPassPtr->getColorTexture(),
                               geometryPassPtr->getDepthTexture(), forwardRenderables, forwardEffectShaderPtr,
                               currentTime);

        unsigned int displayTex = postProcessPassPtr->getColorTexture();
        switch (static_cast<ViewMode>(gbufferViewMode))
        {
        case ViewMode::Final:
            displayTex = postProcessPassPtr->getColorTexture();
            break;
        case ViewMode::Lighting:
            displayTex = lightingPassPtr->getLightingTexture();
            break;
        case ViewMode::Position:
            displayTex = geometryPassPtr->getPositionTexture();
            break;
        case ViewMode::Normal:
            displayTex = geometryPassPtr->getNormalTexture();
            break;
        case ViewMode::Diffuse:
            displayTex = geometryPassPtr->getDiffuseTexture();
            break;
        case ViewMode::Specular:
            displayTex = geometryPassPtr->getSpecularTexture();
            break;
        case ViewMode::Shininess:
            displayTex = geometryPassPtr->getShininessTexture();
            break;
        case ViewMode::Depth:
            displayTex = geometryPassPtr->getDepthTexture();
            break;
        default:
            break;
        }
        finalPassPtr->render(WIN_WIDTH, WIN_HEIGHT, displayTex);
        // finalPass.render(WIN_WIDTH, WIN_HEIGHT, lightingPass.getLightingTexture());
        // finalPass.render(WIN_WIDTH, WIN_HEIGHT, geometryPass.getUIDTexture());

        if (selectedRenderable)
        {
            guiLayer.SetSelectedRenderable(selectedRenderable, currentSelectedUID);
        }
        guiLayer.RenderGUI();
        guiLayer.EndFrame();
        m_window->swapBuffers();
    }
}

void Application::Shutdown()
{
    guiLayer.Shutdown();
    m_window->terminateGLFW();
    LOG_INFO("3DGS Engine 正常退出");
}

GSENGINE_NAMESPACE_END
