#pragma once

#include "Core.h"
#include "Event/EventBus.h"
#include "Gui/GuiLayer.h"
#include "Renderer/Camera.h"
#include "Scene/Scene.h"
#include "Window/Window.h"
#include "Assets/TextureManager.h"
#include "Assets/MaterialManager.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/RenderPipeline.h"
#include <memory>

GSENGINE_NAMESPACE_BEGIN

const int ACTION_RELEASE = 0;
const int ACTION_PRESS = 1;
struct AppConfig
{
    int width = 1920;
    int height = 1080;
    const char *title = "3DGS Engine";
};

struct RenderConfig
{
    int viewMode = static_cast<int>(Renderer::ViewMode::Final);
    float exposure = 1.0f;
    int tonemapMode = 2; // 0 = None, 1 = Reinhard, 2 = ACES Filmic
    bool ssaoEnabled = true;
    bool presentToScreen = true;
    int selectedUID = -1;
    int shadowMapResolution = 4096;
};

class Window;
class Scene;
class GuiLayer;
class EventBus;

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

class GSENGINE_API Application
{
public:
    Application(AppConfig config);
    virtual ~Application();

    // 基础生命周期方法
    bool Init();
    void Run();
    void Shutdown();

    // 虚方法供派生类实现
    virtual bool OnInit();
    virtual void OnShutdown();
    virtual void OnUpdate(float deltaTime);
    virtual void OnRender(float deltaTime);
    virtual void OnHandleInput();
    virtual void OnGUI();
    virtual void OnResize(int width, int height);

protected:
    std::shared_ptr<Window> m_window;
    std::shared_ptr<Scene> m_scene;
    std::shared_ptr<GuiLayer> m_guiLayer;
    std::shared_ptr<EventBus> m_eventBus;
    std::shared_ptr<Renderer::Camera> m_camera;
    std::shared_ptr<TextureManager> m_textureManager;
    std::shared_ptr<MaterialManager> m_materialManager;
    std::shared_ptr<Renderer::ShaderManager> m_shaderManager;
    std::shared_ptr<Renderer::RenderPipeline> m_renderPipeline;

    InputState m_inputState;
    AppConfig m_appConfig;
    RenderConfig m_renderConfig;

    // 基础输入事件处理（可以被派生类扩展）
    virtual void HandleKeyEvent(int key, int scancode, int action, int mods);
    virtual void HandleMouseButtonEvent(int button, int action, int mods, double xpos, double ypos);
    virtual void HandleMouseMoveEvent(double xpos, double ypos, double dx, double dy);
    virtual void HandleScrollEvent(double xoffset, double yoffset);
    virtual void HandleWindowResize(int width, int height);

private:
    // 私有辅助方法
    bool InitWindow();
    bool InitGUI();
    void InitInputHandling();
    void UpdateCamera(float deltaTime);
    void ProcessEvents();
};

GSENGINE_NAMESPACE_END
