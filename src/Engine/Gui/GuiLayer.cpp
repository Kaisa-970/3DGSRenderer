#include "GuiLayer.h"
#include "Assets/MaterialManager.h"
#include "Renderer/MathUtils/Random.h"
#include "Renderer/Material.h"
#include "Renderer/Primitives/SpherePrimitive.h"
#include "Window/Window.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cmath>
#include <cstdint>
#include <string>

GSENGINE_NAMESPACE_BEGIN

GuiLayer::GuiLayer()
{
}

GuiLayer::~GuiLayer()
{
    Shutdown();
}

void GuiLayer::Init(Window *window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    //  放大字体与控件尺寸
    ImGui::GetStyle().ScaleAllSizes(1.5f);
    io.FontGlobalScale = 1.5f;

    auto glfwWindow = static_cast<GLFWwindow *>(window->getNativeHandle());
    ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
    ImGui_ImplOpenGL3_Init();
}

void GuiLayer::BeginFrame()
{
    sceneHovered_ = false;
    sceneFocused_ = false;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GuiLayer::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GuiLayer::RenderGUI()
{
    // 使用标准 DockSpace 全屏承载编辑器布局（可拖拽、可停靠）
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGuiWindowFlags dockHostFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                                     ImGuiWindowFlags_NoDocking;
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("EditorDockHost", nullptr, dockHostFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");

    // 首次启动时构建默认编辑器布局
    if (!dockLayoutInitialized_ && ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
    {
        dockLayoutInitialized_ = true;

        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

        // 左侧 20% 放 Hierarchy
        ImGuiID dockMain = dockspaceId;
        ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.18f, nullptr, &dockMain);
        // 右侧 22% 放 Inspector
        ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.22f, nullptr, &dockMain);

        ImGui::DockBuilderDockWindow("Scene", dockMain);
        ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
        ImGui::DockBuilderDockWindow("Inspector", dockRight);

        ImGui::DockBuilderFinish(dockspaceId);
    }

    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f));
    ImGui::End();

    // 具体面板交由 DockSpace 管理位置与大小
    ImGui::Begin("Scene");
    RenderScenePanel();
    ImGui::End();

    ImGui::Begin("Hierarchy");
    RenderHierarchyPanel();
    ImGui::End();

    ImGui::Begin("Inspector");
    RenderInspectorPanel();
    ImGui::End();
}

void GuiLayer::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

bool GuiLayer::WantCaptureMouse() const
{
    if (!ImGui::GetCurrentContext())
        return false;
    // Scene 面板交互时，把鼠标输入交给相机/场景控制
    if (sceneHovered_ || sceneFocused_)
        return false;
    return ImGui::GetIO().WantCaptureMouse;
}

bool GuiLayer::WantCaptureKeyboard() const
{
    if (!ImGui::GetCurrentContext())
        return false;
    // Scene 面板聚焦时，允许 WASD 等键盘控制
    if (sceneFocused_ || sceneHovered_)
        return false;
    return ImGui::GetIO().WantCaptureKeyboard;
}

void GuiLayer::SetScene(const std::shared_ptr<Scene> &scene)
{
    if (!scene)
        return;
    scene_ = std::weak_ptr<Scene>(scene);
}

void GuiLayer::SetSelectedRenderable(const std::shared_ptr<Renderer::Renderable> &renderable, unsigned int uid)
{
    if (!renderable)
    {
        ClearSelection();
        return;
    }
    selected_ = renderable;
    selectedUid_ = uid;
    SyncEditableFromTransform(*renderable);
}

void GuiLayer::SetGBufferViewModes(int *modePtr, const std::vector<const char *> &labels)
{
    gbufferViewMode_ = modePtr;
    gbufferViewLabels_ = labels;
}

void GuiLayer::SetHDRControls(float *exposurePtr, int *tonemapModePtr)
{
    exposurePtr_ = exposurePtr;
    tonemapModePtr_ = tonemapModePtr;
}

void GuiLayer::SetBloomControls(float *thresholdPtr, float *intensityPtr, int *iterationsPtr, bool *enabledPtr)
{
    bloomThresholdPtr_ = thresholdPtr;
    bloomIntensityPtr_ = intensityPtr;
    bloomIterationsPtr_ = iterationsPtr;
    bloomEnabledPtr_ = enabledPtr;
}

void GuiLayer::SetMaterialManager(const std::shared_ptr<MaterialManager> &materialManager)
{
    if (!materialManager)
        return;
    materialManager_ = std::weak_ptr<MaterialManager>(materialManager);
}

void GuiLayer::SetSceneViewTexture(unsigned int textureId, int texWidth, int texHeight)
{
    sceneViewTexture_ = textureId;
    sceneViewTexWidth_ = texWidth > 0 ? texWidth : 1;
    sceneViewTexHeight_ = texHeight > 0 ? texHeight : 1;
}

void GuiLayer::GetSceneViewportSize(int &width, int &height) const
{
    width = sceneViewportWidth_;
    height = sceneViewportHeight_;
}

void GuiLayer::RenderScenePanel()
{
    sceneHovered_ = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    sceneFocused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    sceneViewportWidth_ = (avail.x > 1.0f) ? static_cast<int>(avail.x) : 1;
    sceneViewportHeight_ = (avail.y > 1.0f) ? static_cast<int>(avail.y) : 1;
    if (sceneViewTexture_ == 0)
    {
        ImGui::TextDisabled("No scene texture.");
        return;
    }
    if (avail.x < 1.0f || avail.y < 1.0f)
        return;

    // 渲染管线已同步到面板尺寸，纹理直接铺满整个面板
    // 记录场景图像在屏幕上的位置（用于鼠标坐标映射）
    ImVec2 screenPos = ImGui::GetCursorScreenPos();
    sceneImageScreenX_ = screenPos.x;
    sceneImageScreenY_ = screenPos.y;
    sceneImageWidth_ = avail.x;
    sceneImageHeight_ = avail.y;

    // OpenGL 纹理坐标原点在左下，ImGui 默认左上，因此 V 方向翻转
    ImGui::Image((ImTextureID)(intptr_t)sceneViewTexture_, avail, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
}

void GuiLayer::RenderHierarchyPanel()
{
    auto scene = scene_.lock();
    if (!scene)
    {
        ImGui::TextDisabled("No scene bound.");
        return;
    }

    if (ImGui::Button("Create Sphere"))
    {
        std::shared_ptr<Renderer::SpherePrimitive> spherePrimitive =
            std::make_shared<Renderer::SpherePrimitive>(1.0f, 36, 18);
        auto sphereRenderable = std::make_shared<Renderer::Renderable>();
        sphereRenderable->setPrimitive(spherePrimitive);
        sphereRenderable->setTransform(Renderer::Matrix4::identity());
        sphereRenderable->setColor(Renderer::Random::randomColor());
        if (auto matMgr = materialManager_.lock())
            sphereRenderable->setMaterial(matMgr->GetDefaultMaterial());
        scene->AddRenderable(sphereRenderable);
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear Selection"))
        ClearSelection();

    ImGui::Separator();
    const auto &renderables = scene->GetRenderables();
    for (const auto &r : renderables)
    {
        if (!r)
            continue;
        unsigned int uid = r->getUid();
        bool isSelected = (selectedUid_ == uid) && !selected_.expired();

        std::string label = "[" + std::to_string(uid) + "] ";
        label += (r->getType() == Renderer::RenderableType::Model) ? "Model" : "Primitive";

        ImGui::PushID(static_cast<int>(uid));
        if (ImGui::Selectable(label.c_str(), isSelected))
        {
            selected_ = r;
            selectedUid_ = uid;
            SyncEditableFromTransform(*r);
        }
        ImGui::PopID();
    }
}

void GuiLayer::RenderInspectorPanel()
{
    if (auto selected = selected_.lock())
    {
        ImGui::Text("UID: %u", selectedUid_);
        ImGui::Text("Type: %s", selected->getType() == Renderer::RenderableType::Model ? "Model" : "Primitive");

        bool changed = false;
        changed |= ImGui::DragFloat3("Position", &editPosition_.x, 0.01f, -FLT_MAX, FLT_MAX, "%.3f");
        changed |= ImGui::DragFloat3("Rotation (deg XYZ)", &editRotationDeg_.x, 0.1f, -360.0f, 360.0f, "%.3f");
        changed |= ImGui::DragFloat3("Scale", &editScale_.x, 0.01f, 0.0001f, FLT_MAX, "%.3f");
        if (ImGui::Button("Reset Transform"))
        {
            editPosition_ = {0.0f, 0.0f, 0.0f};
            editRotationDeg_ = {0.0f, 0.0f, 0.0f};
            editScale_ = {1.0f, 1.0f, 1.0f};
            changed = true;
        }
        if (changed)
            ApplyEditableToRenderable(*selected);

        ImGui::Separator();
        Renderer::Vector3 color = selected->getColor();
        if (ImGui::ColorEdit3("Color", &color.x))
            selected->setColor(color);

        if (selected->getType() == Renderer::RenderableType::Primitive)
        {
            auto mat = selected->getMaterial();
            if (mat)
            {
                ImGui::Separator();
                ImGui::Text("Material: %s", mat->getName().c_str());
                ImGui::Text("Diffuse: (%.3f, %.3f, %.3f)", mat->getDiffuseColor().x, mat->getDiffuseColor().y,
                            mat->getDiffuseColor().z);
                ImGui::Text("Specular: (%.3f, %.3f, %.3f)", mat->getSpecularColor().x, mat->getSpecularColor().y,
                            mat->getSpecularColor().z);
                ImGui::Text("Ambient: (%.3f, %.3f, %.3f)", mat->getAmbientColor().x, mat->getAmbientColor().y,
                            mat->getAmbientColor().z);
                float shininess = mat->getShininess();
                if (ImGui::SliderFloat("Shininess", &shininess, 1.0f, 256.0f, "%.1f"))
                    mat->setShininess(shininess);
                ImGui::Text("Textures: diffuse %zu, specular %zu, normal %zu", mat->getDiffuseTextures().size(),
                            mat->getSpecularTextures().size(), mat->getNormalTextures().size());
            }
        }
        else
        {
            auto model = selected->getModel();
            if (model)
            {
                auto &submeshes = model->getSubMeshes();
                ImGui::Separator();
                ImGui::Text("SubMeshes: %zu", submeshes.size());
                if (!submeshes.empty() && submeshes[0].material)
                {
                    auto mat = submeshes[0].material;
                    ImGui::Text("Material[0] Diffuse: (%.3f, %.3f, %.3f)", mat->getDiffuseColor().x,
                                mat->getDiffuseColor().y, mat->getDiffuseColor().z);
                    ImGui::Text("Material[0] Specular: (%.3f, %.3f, %.3f)", mat->getSpecularColor().x,
                                mat->getSpecularColor().y, mat->getSpecularColor().z);
                    float shininess = mat->getShininess();
                    if (ImGui::SliderFloat("Material[0] Shininess", &shininess, 1.0f, 256.0f, "%.1f"))
                        mat->setShininess(shininess);
                    ImGui::Text("Material[0] Textures: diffuse %zu, specular %zu, normal %zu",
                                mat->getDiffuseTextures().size(), mat->getSpecularTextures().size(),
                                mat->getNormalTextures().size());
                }
            }
        }
    }
    else
    {
        ImGui::TextDisabled("No selection");
    }

    // ---- Rendering Settings（始终显示，不依赖物体选中状态）----
    ImGui::Separator();
    if (gbufferViewMode_ && !gbufferViewLabels_.empty())
    {
        ImGui::Text("G-Buffer View");
        ImGui::Combo("Texture", gbufferViewMode_, gbufferViewLabels_.data(),
                     static_cast<int>(gbufferViewLabels_.size()));
    }

    // HDR / Tone Mapping 控制
    if (exposurePtr_ || tonemapModePtr_)
    {
        ImGui::Separator();
        ImGui::Text("HDR / Tone Mapping");
        if (exposurePtr_)
        {
            ImGui::SliderFloat("Exposure", exposurePtr_, 0.1f, 10.0f, "%.2f");
        }
        if (tonemapModePtr_)
        {
            static const char *tonemapLabels[] = {"None (Clamp)", "Reinhard", "ACES Filmic"};
            ImGui::Combo("Tonemap", tonemapModePtr_, tonemapLabels, 3);
        }
    }

    // Bloom 控制
    if (bloomEnabledPtr_)
    {
        ImGui::Separator();
        ImGui::Text("Bloom");
        ImGui::Checkbox("Enable Bloom", bloomEnabledPtr_);
        if (*bloomEnabledPtr_)
        {
            if (bloomThresholdPtr_)
                ImGui::SliderFloat("Threshold", bloomThresholdPtr_, 0.0f, 5.0f, "%.2f");
            if (bloomIntensityPtr_)
                ImGui::SliderFloat("Intensity", bloomIntensityPtr_, 0.0f, 3.0f, "%.2f");
            if (bloomIterationsPtr_)
                ImGui::SliderInt("Blur Iterations", bloomIterationsPtr_, 1, 20);
        }
    }
}

void GuiLayer::ClearSelection()
{
    selected_.reset();
    selectedUid_ = 0;
    hasEditState_ = false;
}

void GuiLayer::SyncEditableFromTransform(const Renderer::Renderable &renderable)
{
    const Renderer::Transform &transform = renderable.m_transform;

    editPosition_.x = transform.position.x;
    editPosition_.y = transform.position.y;
    editPosition_.z = transform.position.z;

    editScale_.x = transform.scale.x;
    editScale_.y = transform.scale.y;
    editScale_.z = transform.scale.z;

    editRotationDeg_.x = transform.rotation.pitch;
    editRotationDeg_.y = transform.rotation.yaw;
    editRotationDeg_.z = transform.rotation.roll;

    hasEditState_ = true;
}

void GuiLayer::ApplyEditableToRenderable(Renderer::Renderable &renderable)
{
    renderable.m_transform.position = Renderer::Vector3(editPosition_.x, editPosition_.y, editPosition_.z);
    renderable.m_transform.scale = Renderer::Vector3(editScale_.x, editScale_.y, editScale_.z);
    renderable.m_transform.rotation = Renderer::Rotator(editRotationDeg_.x, editRotationDeg_.y, editRotationDeg_.z);
    hasEditState_ = true;
}

bool GuiLayer::WindowToSceneViewport(double windowX, double windowY, int &outX, int &outY) const
{
    if (sceneImageWidth_ <= 0.0f || sceneImageHeight_ <= 0.0f)
        return false;

    // 计算鼠标相对于场景图像左上角的偏移
    float relX = static_cast<float>(windowX) - sceneImageScreenX_;
    float relY = static_cast<float>(windowY) - sceneImageScreenY_;

    // 检查是否在场景图像范围内
    if (relX < 0.0f || relY < 0.0f || relX >= sceneImageWidth_ || relY >= sceneImageHeight_)
        return false;

    // 映射到帧缓冲坐标
    outX = static_cast<int>(relX / sceneImageWidth_ * static_cast<float>(sceneViewTexWidth_));
    // 图像使用 V 翻转显示 (uv0.y=1, uv1.y=0)，所以屏幕 Y 向下对应帧缓冲 Y 向下
    // 但 OpenGL 帧缓冲 Y=0 在底部，所以需要翻转
    outY = static_cast<int>((1.0f - relY / sceneImageHeight_) * static_cast<float>(sceneViewTexHeight_));

    return true;
}

GSENGINE_NAMESPACE_END
