#include "Application.h"
#include "Assets/TextureManager.h"
#include "Assets/MaterialManager.h"
#include "Event/EventBus.h"
#include "Gui/GuiLayer.h"
#include "Logger/Log.h"
#include "Renderer/Camera.h"
#include "Scene/Scene.h"
#include "Window/Window.h"
#include "Renderer/RenderPipeline.h"

GSENGINE_NAMESPACE_BEGIN

static const Renderer::Vector3 DEFAULT_CAM_POS(0.0f, 1.0f, 3.0f);

Application::Application(AppConfig config) : m_appConfig(config)
{
    m_eventBus = std::make_shared<EventBus>();
    m_scene = std::make_shared<Scene>();
    m_camera = std::make_shared<Renderer::Camera>(DEFAULT_CAM_POS, Renderer::Vector3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
    m_camera->setMovementSpeed(2.0f);
    m_camera->setMouseSensitivity(0.1f);
}

Application::~Application()
{
}

bool Application::Init()
{
    // 初始化日志系统
    Logger::Log::Init();
    LOG_INFO("3DGS Engine 启动中...");

    // 初始化基础组件
    if (!InitWindow())
    {
        LOG_ERROR("窗口初始化失败");
        return false;
    }

    // 创建资源管理器（窗口/GL上下文就绪后）
    m_textureManager = std::make_shared<TextureManager>();
    m_materialManager = std::make_shared<MaterialManager>(*m_textureManager);
    m_shaderManager = std::make_shared<Renderer::ShaderManager>();

    m_renderPipeline =
        std::make_shared<Renderer::RenderPipeline>(m_appConfig.width, m_appConfig.height, *m_shaderManager);

    InitInputHandling();

    if (!InitGUI())
    {
        LOG_ERROR("GUI初始化失败");
        return false;
    }

    // 调用派生类的初始化
    if (!OnInit())
    {
        LOG_ERROR("应用初始化失败");
        return false;
    }

    LOG_INFO("Application Init Success!");
    return true;
}

void Application::HandleKeyEvent(int key, int scancode, int action, int mods)
{
    bool pressed = action != ACTION_RELEASE;
    if (key == static_cast<int>(Key::W))
        m_inputState.moveForward = pressed;
    if (key == static_cast<int>(Key::S))
        m_inputState.moveBackward = pressed;
    if (key == static_cast<int>(Key::A))
        m_inputState.moveLeft = pressed;
    if (key == static_cast<int>(Key::D))
        m_inputState.moveRight = pressed;
    if (key == static_cast<int>(Key::Q))
        m_inputState.moveDown = pressed;
    if (key == static_cast<int>(Key::E))
        m_inputState.moveUp = pressed;
    if (key == static_cast<int>(Key::Escape) && pressed)
        m_inputState.exitRequested = true;
}

void Application::HandleMouseButtonEvent(int button, int action, int mods, double xpos, double ypos)
{
    if (button == static_cast<int>(MouseButton::Left))
    {
        m_inputState.leftMouseDown = action != ACTION_RELEASE;
        if (action == ACTION_PRESS)
            m_inputState.pickRequested = true;
    }
    if (button == static_cast<int>(MouseButton::Right))
    {
        m_inputState.rightMouseDown = action != ACTION_RELEASE;
        m_inputState.firstMouse = (action == ACTION_RELEASE);
        m_inputState.lastX = xpos;
        m_inputState.lastY = ypos;
    }
}

void Application::HandleMouseMoveEvent(double xpos, double ypos, double dx, double dy)
{
    m_inputState.mouseX = xpos;
    m_inputState.mouseY = ypos;
    if (!m_inputState.rightMouseDown)
        return;
    m_camera->processMouseMovement(static_cast<float>(dx), static_cast<float>(dy));
}

void Application::HandleScrollEvent(double xoffset, double yoffset)
{
    m_camera->processMouseScroll(static_cast<float>(yoffset));
}

void Application::HandleWindowResize(int width, int height)
{
    if (width <= 0 || height <= 0)
        return; // 窗口最小化时 framebuffer 尺寸可能为 0，跳过

    m_appConfig.width = width;
    m_appConfig.height = height;

    LOG_INFO("窗口大小变化: {}x{}", width, height);

    // 通知派生类
    OnResize(width, height);
}

bool Application::InitWindow()
{
    // 初始化 GLFW
    if (!Window::initGLFW())
    {
        LOG_ERROR("初始化 GLFW 失败");
        return false;
    }
    LOG_INFO("GLFW 初始化成功");

    // 创建窗口
    m_window = std::make_shared<Window>(m_appConfig.width, m_appConfig.height, m_appConfig.title);
    if (!m_window)
    {
        LOG_ERROR("创建窗口失败");
        return false;
    }
    LOG_INFO("窗口创建成功: {}x{}", m_appConfig.width, m_appConfig.height);

    return true;
}

bool Application::InitGUI()
{
    m_guiLayer = std::make_shared<GuiLayer>();
    m_guiLayer->Init(m_window.get());
    return true;
}

void Application::InitInputHandling()
{
    // 设置事件回调
    m_window->setKeyCallback([this](int key, int scancode, int action, int mods) {
        m_eventBus->Emplace<KeyEvent>(key, scancode, action, mods, Window::getTime());
    });

    m_window->setMouseButtonCallback([this](int button, int action, int mods, double xpos, double ypos) {
        m_inputState.mouseX = xpos;
        m_inputState.mouseY = ypos;
        m_eventBus->Emplace<MouseButtonEvent>(button, action, mods, xpos, ypos, Window::getTime());
    });

    m_window->setMouseMoveCallback([this](double xpos, double ypos) {
        double dx = 0.0;
        double dy = 0.0;
        if (m_inputState.firstMouse)
        {
            m_inputState.lastX = xpos;
            m_inputState.lastY = ypos;
            m_inputState.firstMouse = false;
        }
        else
        {
            dx = xpos - m_inputState.lastX;
            dy = m_inputState.lastY - ypos;
            if (std::abs(dx) > 100.0 || std::abs(dy) > 100.0)
            {
                m_inputState.lastX = xpos;
                m_inputState.lastY = ypos;
                return;
            }
            m_inputState.lastX = xpos;
            m_inputState.lastY = ypos;
        }
        m_inputState.mouseX = xpos;
        m_inputState.mouseY = ypos;
        m_eventBus->Emplace<MouseMoveEvent>(xpos, ypos, dx, dy, Window::getTime());
    });

    m_window->setMouseScrollCallback([this](double xoffset, double yoffset) {
        m_eventBus->Emplace<ScrollEvent>(xoffset, yoffset, Window::getTime());
    });

    m_window->setFramebufferSizeCallback(
        [this](int width, int height) { m_eventBus->Emplace<WindowResizeEvent>(width, height); });

    // 注册基础输入事件处理
    m_eventBus->Subscribe(EventType::Key, 10, [this](Event &evt) {
        auto &e = static_cast<KeyEvent &>(evt);
        if (m_guiLayer->WantCaptureKeyboard())
            return;
        HandleKeyEvent(e.key, e.scancode, e.action, e.mods);
    });

    m_eventBus->Subscribe(EventType::MouseButton, 10, [this](Event &evt) {
        auto &e = static_cast<MouseButtonEvent &>(evt);
        if (m_guiLayer->WantCaptureMouse())
            return;
        HandleMouseButtonEvent(e.button, e.action, e.mods, e.x, e.y);
    });

    m_eventBus->Subscribe(EventType::MouseMove, 10, [this](Event &evt) {
        auto &e = static_cast<MouseMoveEvent &>(evt);
        if (m_guiLayer->WantCaptureMouse())
            return;
        HandleMouseMoveEvent(e.x, e.y, e.dx, e.dy);
    });

    m_eventBus->Subscribe(EventType::Scroll, 10, [this](Event &evt) {
        auto &e = static_cast<ScrollEvent &>(evt);
        if (m_guiLayer->WantCaptureMouse())
            return;
        HandleScrollEvent(e.xoffset, e.yoffset);
    });

    m_eventBus->Subscribe(EventType::WindowResize, 10, [this](Event &evt) {
        auto &e = static_cast<WindowResizeEvent &>(evt);
        HandleWindowResize(e.width, e.height);
    });
}

void Application::Run()
{
    float lastTime = static_cast<float>(m_window->getTime());
    float startTime = lastTime;
    int frameCount = 0;

    while (!m_window->shouldClose())
    {
        float currentTime = static_cast<float>(m_window->getTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // FPS计算
        frameCount++;
        if (currentTime - startTime >= 1.0f)
        {
            float fps = static_cast<float>(frameCount) / (currentTime - startTime);
            LOG_INFO("FPS: {}", int(fps));
            startTime = currentTime;
            frameCount = 0;
        }

        // 处理事件
        ProcessEvents();

        // 检查退出条件
        if (m_inputState.exitRequested)
            break;

        // 窗口最小化时跳过渲染，避免 framebuffer 为 0 导致的异常
        if (m_window->getWidth() <= 0 || m_window->getHeight() <= 0)
            continue;

        // 更新相机
        UpdateCamera(deltaTime);

        // 调用派生类的更新逻辑
        OnUpdate(deltaTime);

        // GUI开始帧
        m_guiLayer->BeginFrame();

        // 渲染
        m_renderPipeline->Execute(*m_camera, m_scene->GetRenderables(), m_scene->GetLights(), -1,
                                  Renderer::ViewMode::Final, 0, true);

        // 调用派生类的渲染逻辑
        OnRender(deltaTime);

        // 调用派生类的GUI逻辑
        OnGUI();

        // GUI结束帧
        m_guiLayer->EndFrame();

        // 交换缓冲区
        m_window->swapBuffers();
    }
}

void Application::ProcessEvents()
{
    m_window->pollEvents();
    m_eventBus->Dispatch();
}

void Application::UpdateCamera(float deltaTime)
{
    if (m_inputState.moveForward)
        m_camera->processKeyboard(Renderer::CameraMovement::Forward, deltaTime);
    if (m_inputState.moveBackward)
        m_camera->processKeyboard(Renderer::CameraMovement::Backward, deltaTime);
    if (m_inputState.moveLeft)
        m_camera->processKeyboard(Renderer::CameraMovement::Left, deltaTime);
    if (m_inputState.moveRight)
        m_camera->processKeyboard(Renderer::CameraMovement::Right, deltaTime);
    if (m_inputState.moveUp)
        m_camera->processKeyboard(Renderer::CameraMovement::Up, deltaTime);
    if (m_inputState.moveDown)
        m_camera->processKeyboard(Renderer::CameraMovement::Down, deltaTime);
}

void Application::Shutdown()
{
    // 先关闭 GUI 后端，避免后续资源释放时仍访问 ImGui 相关状态
    if (m_guiLayer)
        m_guiLayer->Shutdown();

    // 先让派生类释放自身持有的渲染资源（例如 RenderPipeline）
    OnShutdown();

    // 再释放基础资源管理器和场景对象，确保 GL 资源析构发生在上下文仍然有效时
    m_shaderManager.reset();
    m_materialManager.reset();
    m_textureManager.reset();
    m_scene.reset();
    m_camera.reset();
    m_eventBus.reset();
    m_guiLayer.reset();
    m_renderPipeline.reset();

    // 最后销毁窗口并终止 GLFW
    m_window.reset();
    Window::terminateGLFW();

    LOG_INFO("3DGS Engine 正常退出");
}

bool Application::OnInit()
{
    return true;
}
void Application::OnShutdown()
{
}
void Application::OnUpdate(float deltaTime)
{
}
void Application::OnRender(float deltaTime)
{
}
void Application::OnHandleInput()
{
}
void Application::OnGUI()
{
}
void Application::OnResize(int width, int height)
{
}
GSENGINE_NAMESPACE_END
