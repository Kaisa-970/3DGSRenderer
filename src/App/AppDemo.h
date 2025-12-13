#pragma once

#include "Engine/Application.h"
#include "Renderer/Primitives/CubePrimitive.h"
#include "Renderer/Primitives/SpherePrimitive.h"
#include "Renderer/Primitives/QuadPrimitive.h"
#include "Renderer/Material.h"
#include "Renderer/Model.h"
#include <memory>

class AppDemo : public GSEngine::Application
{
public:
    AppDemo(GSEngine::AppConfig config);
    virtual ~AppDemo();

    // 实现基类的虚函数
    virtual bool OnInit() override;
    virtual void OnUpdate(float deltaTime) override;
    virtual void OnRender(float deltaTime) override;
    virtual void OnGUI() override;

    // 扩展输入处理
    virtual void HandleKeyEvent(int key, int scancode, int action, int mods) override;

private:
    // 场景设置
    void SetupScene(
        std::shared_ptr<Renderer::CubePrimitive> cubePrimitive,
        std::shared_ptr<Renderer::SpherePrimitive> spherePrimitive,
        std::shared_ptr<Renderer::QuadPrimitive> quadPrimitive,
        std::shared_ptr<Renderer::Material> defaultMaterial,
        std::shared_ptr<Renderer::Model> loadedModel,
        std::shared_ptr<Renderer::Model> loadedModel2
    );

    // PIMPL模式实现细节
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
