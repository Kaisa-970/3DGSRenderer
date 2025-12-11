#pragma once

#include "Core/RenderCore.h"
#include "MathUtils/Vector.h"
#include <memory>
#include <vector>

RENDERER_NAMESPACE_BEGIN

class Window;
class Renderable;
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

    void SetSelectedRenderable(const std::shared_ptr<Renderable>& renderable, unsigned int uid);
    void SetGBufferViewModes(int* modePtr, const std::vector<const char*>& labels);

private:
    void SyncEditableFromTransform(const Renderable& renderable);
    void ApplyEditableToRenderable(Renderable& renderable);

    std::weak_ptr<Renderable> selected_;
    unsigned int selectedUid_{0};
    Vector3 editPosition_{0.0f, 0.0f, 0.0f};
    Vector3 editRotationDeg_{0.0f, 0.0f, 0.0f}; // pitch(y), yaw(x), roll(z) approximate
    Vector3 editScale_{1.0f, 1.0f, 1.0f};
    bool hasEditState_{false};
    int* gbufferViewMode_{nullptr};
    std::vector<const char*> gbufferViewLabels_;
};

RENDERER_NAMESPACE_END