#include "GuiLayer.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/MathUtils/Random.h"
#include "Renderer/Primitives/SpherePrimitive.h"
#include "Window/Window.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cmath>


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
    // ImGui::ShowDemoWindow();

    if (auto scene = scene_.lock())
    {
        ImGui::Begin("Settings");
        if (ImGui::Button("Create Sphere"))
        {
            std::shared_ptr<Renderer::SpherePrimitive> spherePrimitive =
                std::make_shared<Renderer::SpherePrimitive>(1.0f, 36, 18);
            auto sphereRenderable = std::make_shared<Renderer::Renderable>();
            sphereRenderable->setPrimitive(spherePrimitive);
            sphereRenderable->setTransform(Renderer::Matrix4::identity());
            Renderer::Vector3 color = Renderer::Random::randomColor();
            sphereRenderable->setColor(color);
            sphereRenderable->setMaterial(Renderer::MaterialManager::GetInstance()->GetDefaultMaterial());
            scene->AddRenderable(sphereRenderable);
        }

        ImGui::End();
    }

    if (auto selected = selected_.lock())
    {
        ImGui::Begin("Selection");
        ImGui::Text("UID: %u", selectedUid_);
        ImGui::Text("Type: %s", selected->getType() == Renderer::RenderableType::Model ? "Model" : "Primitive");

        // 变换可编辑
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
        {
            ApplyEditableToRenderable(*selected);
        }

        ImGui::Separator();
        Renderer::Vector3 color = selected->getColor();
        if (ImGui::ColorEdit3("Color", &color.x))
        {
            selected->setColor(color);
        }

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
                {
                    mat->setShininess(shininess);
                }
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
                    {
                        mat->setShininess(shininess);
                    }
                    ImGui::Text("Material[0] Textures: diffuse %zu, specular %zu, normal %zu",
                                mat->getDiffuseTextures().size(), mat->getSpecularTextures().size(),
                                mat->getNormalTextures().size());
                }
            }
        }

        ImGui::Separator();
        if (gbufferViewMode_ && !gbufferViewLabels_.empty())
        {
            ImGui::Text("G-Buffer View");
            ImGui::Combo("Texture", gbufferViewMode_, gbufferViewLabels_.data(),
                         static_cast<int>(gbufferViewLabels_.size()));
        }
        ImGui::End();
    }
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
    return ImGui::GetIO().WantCaptureMouse;
}

bool GuiLayer::WantCaptureKeyboard() const
{
    if (!ImGui::GetCurrentContext())
        return false;
    return ImGui::GetIO().WantCaptureKeyboard;
}

void GuiLayer::SetScene(const std::shared_ptr<Renderer::Scene> &scene)
{
    if (!scene)
        return;
    scene_ = std::weak_ptr<Renderer::Scene>(scene);
}

void GuiLayer::SetSelectedRenderable(const std::shared_ptr<Renderer::Renderable> &renderable, unsigned int uid)
{
    if (!renderable)
        return; // 保留上一选中
    selected_ = renderable;
    selectedUid_ = uid;
    SyncEditableFromTransform(*renderable);
}

void GuiLayer::SetGBufferViewModes(int *modePtr, const std::vector<const char *> &labels)
{
    gbufferViewMode_ = modePtr;
    gbufferViewLabels_ = labels;
}

void GuiLayer::SyncEditableFromTransform(const Renderer::Renderable &renderable)
{
    const auto &m = renderable.getTransform();
    // 假定存储为列主序（构建后转置过），平移在 m[12/13/14]
    editPosition_.x = m.m[12];
    editPosition_.y = m.m[13];
    editPosition_.z = m.m[14];

    // 提取缩放（列向量长度）
    auto len = [](float a, float b, float c) { return std::sqrt(a * a + b * b + c * c); };
    editScale_.x = len(m.m[0], m.m[1], m.m[2]);
    editScale_.y = len(m.m[4], m.m[5], m.m[6]);
    editScale_.z = len(m.m[8], m.m[9], m.m[10]);
    // 防止零尺度
    if (editScale_.x == 0)
        editScale_.x = 1.0f;
    if (editScale_.y == 0)
        editScale_.y = 1.0f;
    if (editScale_.z == 0)
        editScale_.z = 1.0f;

    // 归一化旋转矩阵
    float r00 = m.m[0] / editScale_.x;
    float r01 = m.m[4] / editScale_.y;
    float r02 = m.m[8] / editScale_.z;
    float r10 = m.m[1] / editScale_.x;
    float r11 = m.m[5] / editScale_.y;
    float r12 = m.m[9] / editScale_.z;
    float r20 = m.m[2] / editScale_.x;
    float r21 = m.m[6] / editScale_.y;
    float r22 = m.m[10] / editScale_.z;
    (void)r01;
    (void)r02;
    (void)r11;
    (void)r12; // 当前欧拉提取未用到其余分量

    // 简单 XYZ 欧拉角提取（行主序）
    float pitch = std::asin(-r20);
    float yaw = std::atan2(r10, r00);
    float roll = std::atan2(r21, r22);
    const float rad2deg = 180.0f / 3.1415926f;
    editRotationDeg_.x = yaw * rad2deg;
    editRotationDeg_.y = pitch * rad2deg;
    editRotationDeg_.z = roll * rad2deg;
    hasEditState_ = true;
}

void GuiLayer::ApplyEditableToRenderable(Renderer::Renderable &renderable)
{
    // 以顺序: Scale -> RotX -> RotY -> RotZ ->
    // Translate，然后转置以匹配列主序存储
    Renderer::Matrix4 model = Renderer::Matrix4::identity();
    model.scaleBy(editScale_.x, editScale_.y, editScale_.z);
    const float deg2rad = 3.1415926f / 180.0f;
    model.rotate(editRotationDeg_.x * deg2rad, Renderer::Vector3(1.0f, 0.0f, 0.0f));
    model.rotate(editRotationDeg_.y * deg2rad, Renderer::Vector3(0.0f, 1.0f, 0.0f));
    model.rotate(editRotationDeg_.z * deg2rad, Renderer::Vector3(0.0f, 0.0f, 1.0f));
    model.translate(editPosition_.x, editPosition_.y, editPosition_.z);
    renderable.setTransform(model.transpose());
    hasEditState_ = true;
}

GSENGINE_NAMESPACE_END
