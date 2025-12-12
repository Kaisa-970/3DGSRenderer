#pragma once

#include "Core.h"
#include "MathUtils/Vector.h"
#include "Renderer/Scene.h"
#include <memory>
#include <vector>

GSENGINE_NAMESPACE_BEGIN

class Window;
class GSENGINE_API GuiLayer
{
public:
    GuiLayer();
    ~GuiLayer();

    void Init(Window *window);
    void BeginFrame();
    void EndFrame();
    void RenderGUI();
    void Shutdown();

    bool WantCaptureMouse() const;
    bool WantCaptureKeyboard() const;

    void SetScene(const std::shared_ptr<Renderer::Scene> &scene);
    void SetSelectedRenderable(const std::shared_ptr<Renderer::Renderable> &renderable, unsigned int uid);
    void SetGBufferViewModes(int *modePtr, const std::vector<const char *> &labels);

private:
    void SyncEditableFromTransform(const Renderer::Renderable &renderable);
    void ApplyEditableToRenderable(Renderer::Renderable &renderable);

    std::weak_ptr<Renderer::Scene> scene_;
    std::weak_ptr<Renderer::Renderable> selected_;
    unsigned int selectedUid_{0};
    Renderer::Vector3 editPosition_{0.0f, 0.0f, 0.0f};
    Renderer::Vector3 editRotationDeg_{0.0f, 0.0f, 0.0f}; // pitch(y), yaw(x), roll(z) approximate
    Renderer::Vector3 editScale_{1.0f, 1.0f, 1.0f};
    bool hasEditState_{false};
    int *gbufferViewMode_{nullptr};
    std::vector<const char *> gbufferViewLabels_;
};

GSENGINE_NAMESPACE_END
