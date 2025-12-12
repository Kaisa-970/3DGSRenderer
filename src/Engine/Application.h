#pragma once

#include "Core.h"

GSENGINE_NAMESPACE_BEGIN

struct AppConfig
{
    int width = 1920;
    int height = 1080;
    const char *title = "3DGS Engine";
};

class Window;

class GSENGINE_API Application
{
public:
    Application(AppConfig config);
    ~Application();

    bool Init();
    void Run();
    void Shutdown();

    // private:
    //     void Init();
    //     void Update();
    //     void Render();
    //     void Cleanup();

private:
    AppConfig m_appConfig;
    Window *m_window = nullptr;
    // bool m_isRunning;
    // bool m_isPaused;
    // bool m_isMinimized;
    // bool m_isMaximized;
    // bool m_isFullscreen;
    // bool m_isBorderless;
    // bool m_isResizable;
};

GSENGINE_NAMESPACE_END
