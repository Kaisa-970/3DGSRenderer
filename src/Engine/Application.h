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

class Window;
class Scene;
class GuiLayer;
class EventBus;
class Camera;

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
    virtual bool OnInit() { return true; }                    // 应用特定的初始化
    virtual void OnShutdown() {}                              // 应用特定的清理（在基础资源释放前调用）
    virtual void OnUpdate(float deltaTime) {}                // 每帧更新逻辑
    virtual void OnRender(float deltaTime) {}                // 渲染逻辑
    virtual void OnHandleInput() {}                          // 输入处理
    virtual void OnGUI() {}                                  // GUI渲染

protected:
    std::shared_ptr<Window> m_window;
    std::shared_ptr<Scene> m_scene;
    std::shared_ptr<GuiLayer> m_guiLayer;
    std::shared_ptr<EventBus> m_eventBus;
    std::shared_ptr<Renderer::Camera> m_camera;
    std::shared_ptr<TextureManager> m_textureManager;
    std::shared_ptr<MaterialManager> m_materialManager;
    std::shared_ptr<Renderer::ShaderManager> m_shaderManager;

    InputState m_inputState;
    AppConfig m_appConfig;

    // 基础输入事件处理（可以被派生类扩展）
    virtual void HandleKeyEvent(int key, int scancode, int action, int mods);
    virtual void HandleMouseButtonEvent(int button, int action, int mods, double xpos, double ypos);
    virtual void HandleMouseMoveEvent(double xpos, double ypos, double dx, double dy);
    virtual void HandleScrollEvent(double xoffset, double yoffset);

private:
    // 私有辅助方法
    bool InitWindow();
    bool InitGUI();
    void InitInputHandling();
    void UpdateCamera(float deltaTime);
    void ProcessEvents();
};

GSENGINE_NAMESPACE_END
