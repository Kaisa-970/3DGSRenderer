#pragma once

#include "Core/RenderCore.h"

RENDERER_NAMESPACE_BEGIN

class Window;
class RENDERER_API GuiLayer {
public:
    GuiLayer();
    ~GuiLayer();

    void Init(Window* window);
    void BeginFrame();
    void EndFrame();
    void RenderGUI();
    void Shutdown();

    bool WantCaptureMouse() const;
    bool WantCaptureKeyboard() const;
};

RENDERER_NAMESPACE_END