#include "Logger/Log.h"
#include "Renderer/Camera.h"
#include "Renderer/Event/EventBus.h"
#include "Renderer/FinalPass.h"
#include "Renderer/ForwardPass.h"
#include "Renderer/GaussianSplatting/GaussianRenderer.h"
#include "Renderer/GeometryPass.h"
#include "Renderer/Gui/GuiLayer.h"
#include "Renderer/LightingPass.h"
#include "Renderer/MathUtils/Random.h"
#include "Renderer/Model.h"
#include "Renderer/PostProcessPass.h"
#include "Renderer/Primitives/CubePrimitive.h"
#include "Renderer/Primitives/QuadPrimitive.h"
#include "Renderer/Primitives/SpherePrimitive.h"
#include "Renderer/Renderable.h"
#include "Renderer/Scene.h"
#include "Renderer/Window.h"
#include <cmath>

#ifdef GSRENDERER_OS_WINDOWS
#include <windows.h>
#endif

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

constexpr int ACTION_RELEASE = 0;
constexpr int ACTION_PRESS = 1;

constexpr float DEG_TO_RAD = 3.1415926f / 180.0f;
#define DEG2RAD(x) (x * DEG_TO_RAD)

#ifdef RENDERER_DEBUG
std::string pointCloudPath = "E:\\Models\\models\\bicycle\\point_cloud\\iteration_7000\\point_cloud.ply";
std::string modelPath = "./res/backpack/backpack.obj";
std::string model2Path = "./res/SFL-CDD14_Max.fbx";
// std::string model2Path = "./res/monkey.glb";
#else
std::string pointCloudPath = "";
#endif

const int WIN_WIDTH = 2560;
const int WIN_HEIGHT = 1440;

int main(int argc, char *argv[])
{
#ifdef GSRENDERER_OS_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 初始化日志系统
    Logger::Log::Init();

    LOG_INFO("3DGS Renderer 启动中...");

    try
    {
        // 初始化 GLFW
        if (!Renderer::Window::initGLFW())
        {
            LOG_ERROR("初始化 GLFW 失败");
            return -1;
        }
        LOG_INFO("GLFW 初始化成功");

        // 创建窗口（自动初始化 OpenGL 上下文）
        Renderer::Window window(WIN_WIDTH, WIN_HEIGHT, "3DGS Renderer");
        LOG_INFO("窗口创建成功: {}x{}", WIN_WIDTH, WIN_HEIGHT);

        Renderer::EventBus eventBus;
        InputState inputState;
        // 先注册回调，后续 ImGui 安装的回调会自动进行链式转发
        window.setKeyCallback([&eventBus](int key, int scancode, int action, int mods) {
            eventBus.Emplace<Renderer::KeyEvent>(key, scancode, action, mods, Renderer::Window::getTime());
        });
        window.setMouseButtonCallback([&eventBus, &inputState](int button, int action, int mods, double xpos,
                                                               double ypos) {
            inputState.mouseX = xpos;
            inputState.mouseY = ypos;
            eventBus.Emplace<Renderer::MouseButtonEvent>(button, action, mods, xpos, ypos, Renderer::Window::getTime());
        });
        window.setMouseMoveCallback([&eventBus, &inputState](double xpos, double ypos) {
            double dx = 0.0;
            double dy = 0.0;
            if (inputState.firstMouse)
            {
                inputState.lastX = xpos;
                inputState.lastY = ypos;
                inputState.firstMouse = false;
            }
            else
            {
                dx = xpos - inputState.lastX;
                dy = inputState.lastY - ypos;
                if (std::abs(dx) > 100.0 || std::abs(dy) > 100.0)
                {
                    inputState.lastX = xpos;
                    inputState.lastY = ypos;
                    return;
                }
                inputState.lastX = xpos;
                inputState.lastY = ypos;
            }
            inputState.mouseX = xpos;
            inputState.mouseY = ypos;
            eventBus.Emplace<Renderer::MouseMoveEvent>(xpos, ypos, dx, dy, Renderer::Window::getTime());
        });
        window.setMouseScrollCallback([&eventBus](double xoffset, double yoffset) {
            eventBus.Emplace<Renderer::ScrollEvent>(xoffset, yoffset, Renderer::Window::getTime());
        });

        Renderer::GuiLayer guiLayer;
        guiLayer.Init(&window);
        enum class ViewMode
        {
            Final = 0,
            Lighting,
            Position,
            Normal,
            Diffuse,
            Specular,
            Shininess,
            Depth,
            Gaussian
        };
        int gbufferViewMode = static_cast<int>(ViewMode::Final);
        std::vector<const char *> gbufferViewLabels = {
            "Final (PostProcess)", "Lighting", "Position", "Normal", "Diffuse", "Specular",
            "Shininess",           "Depth",    "Gaussian"};
        guiLayer.SetGBufferViewModes(&gbufferViewMode, gbufferViewLabels);
        if (argc < 2)
        {
#ifndef RENDERER_DEBUG
            LOG_ERROR("Usage: 3DGSRenderer <model_path>");
            return -1;
#endif
        }
        else
        {
#ifndef RENDERER_DEBUG
            pointCloudPath = std::string(argv[1]);
#endif
        }
        LOG_INFO("使用模型文件: {}", pointCloudPath);

        Renderer::GaussianRenderer gaussianRenderer;
        // gaussianRenderer.loadModel("res/input.ply");
        gaussianRenderer.loadModel(pointCloudPath);

        std::shared_ptr<Renderer::Model> loadedModel = Renderer::Model::LoadModelFromFile(modelPath);
        std::shared_ptr<Renderer::Model> loadedModel2 = Renderer::Model::LoadModelFromFile(model2Path);
        // 创建相机（根据模型自动计算位置）
        // 模型中心约在 (-4.39, -4.85, -3.90)，尺寸约 48.50
        // 将相机放在模型前方，距离约为尺寸的1.5倍
        Renderer::Vector3 camPos(4.42f, 1.0f, -3.63f);
        Renderer::Camera camera(camPos,                              // 位置（模型中心前方）
                                Renderer::Vector3(0.0f, 1.0f, 0.0f), // 世界上方向
                                133.5f,                              // yaw: -90度 朝向 -Z 方向（看向模型）
                                -14.0f                               // pitch: 0度 水平视角
        );

        camera.setMovementSpeed(2.0f); // 增大移动速度，因为场景较大
        camera.setMouseSensitivity(0.1f);

        float x, y, z;
        camera.getPosition(x, y, z);
        LOG_INFO("相机创建成功，位置: ({}, {}, {})", x, y, z);

        eventBus.Subscribe(Renderer::EventType::Key, 10, [&](Renderer::Event &evt) {
            auto &e = static_cast<Renderer::KeyEvent &>(evt);
            if (guiLayer.WantCaptureKeyboard())
                return;
            bool pressed = e.action != ACTION_RELEASE;
            if (e.key == static_cast<int>(Renderer::Key::W))
                inputState.moveForward = pressed;
            if (e.key == static_cast<int>(Renderer::Key::S))
                inputState.moveBackward = pressed;
            if (e.key == static_cast<int>(Renderer::Key::A))
                inputState.moveLeft = pressed;
            if (e.key == static_cast<int>(Renderer::Key::D))
                inputState.moveRight = pressed;
            if (e.key == static_cast<int>(Renderer::Key::Q))
                inputState.moveDown = pressed;
            if (e.key == static_cast<int>(Renderer::Key::E))
                inputState.moveUp = pressed;
            if (e.key == static_cast<int>(Renderer::Key::Escape) && pressed)
                inputState.exitRequested = true;
            if (e.key == static_cast<int>(Renderer::Key::P) && e.action == ACTION_PRESS)
            {
                inputState.togglePoints = !inputState.togglePoints;
                LOG_INFO("切换点云渲染模式: {}", inputState.togglePoints ? "Points" : "Splats");
            }
        });

        eventBus.Subscribe(Renderer::EventType::MouseButton, 10, [&](Renderer::Event &evt) {
            auto &e = static_cast<Renderer::MouseButtonEvent &>(evt);
            if (guiLayer.WantCaptureMouse())
                return;
            if (e.button == static_cast<int>(Renderer::MouseButton::Left))
            {
                inputState.leftMouseDown = e.action != ACTION_RELEASE;
                if (e.action == ACTION_PRESS)
                    inputState.pickRequested = true;
            }
            if (e.button == static_cast<int>(Renderer::MouseButton::Right))
            {
                inputState.rightMouseDown = e.action != ACTION_RELEASE;
                inputState.firstMouse = (e.action == ACTION_RELEASE);
                inputState.lastX = e.x;
                inputState.lastY = e.y;
            }
        });

        eventBus.Subscribe(Renderer::EventType::MouseMove, 10, [&](Renderer::Event &evt) {
            auto &e = static_cast<Renderer::MouseMoveEvent &>(evt);
            if (guiLayer.WantCaptureMouse())
                return;
            inputState.mouseX = e.x;
            inputState.mouseY = e.y;
            if (!inputState.rightMouseDown)
                return;
            camera.processMouseMovement(static_cast<float>(e.dx), static_cast<float>(e.dy));
        });

        eventBus.Subscribe(Renderer::EventType::Scroll, 10, [&](Renderer::Event &evt) {
            auto &e = static_cast<Renderer::ScrollEvent &>(evt);
            if (guiLayer.WantCaptureMouse())
                return;
            camera.processMouseScroll(static_cast<float>(e.yoffset));
        });

        // 先设置光标模式为禁用（FPS模式），这会锁定并隐藏鼠标
        // window.setCursorMode(Renderer::Window::CursorMode::Disabled);

        // 创建彩色立方体
        Renderer::CubePrimitive cubePrimitive(1.0f);

        Renderer::SpherePrimitive spherePrimitive(1.0f, 64, 32);

        Renderer::QuadPrimitive quadPrimitive(10.0f);

        // 测试相机矩阵
        float viewMatrix[16];
        float projMatrix[16];
        camera.getViewMatrix(viewMatrix);
        camera.getPerspectiveMatrix(projMatrix, 50.0f, static_cast<float>(WIN_WIDTH) / static_cast<float>(WIN_HEIGHT),
                                    0.01f, 1000.0f);

        // 渲染循环

        LOG_INFO("=== 相机控制 ===");
        LOG_INFO("这是一个基本的相机测试");
        LOG_INFO("按 Ctrl+C 退出");
        LOG_INFO("===================");

        Renderer::FinalPass finalPass;
        Renderer::GeometryPass geometryPass(WIN_WIDTH, WIN_HEIGHT);
        Renderer::LightingPass lightingPass(WIN_WIDTH, WIN_HEIGHT);
        Renderer::PostProcessPass postProcessPass(WIN_WIDTH, WIN_HEIGHT);
        Renderer::ForwardPass forwardPass;
        Renderer::Shader forwardEffectShader =
            Renderer::Shader::fromFiles("res/shaders/forward_effect.vs.glsl", "res/shaders/forward_effect.fs.glsl");
        std::shared_ptr<Renderer::Scene> scene = std::make_shared<Renderer::Scene>();
        guiLayer.SetScene(scene);
        auto makePrimitiveRef = [](Renderer::Primitive *prim) {
            return std::shared_ptr<Renderer::Primitive>(prim, [](Renderer::Primitive *) {});
        };
        std::vector<std::shared_ptr<Renderer::Renderable>> sphereRenderables;
        std::vector<std::shared_ptr<Renderer::Renderable>> forwardRenderables;
        float minX = -10.0f;
        float maxX = 10.0f;
        float minY = -10.0f;
        float maxY = 10.0f;
        float minZ = -10.0f;
        float maxZ = 10.0f;
        float minRadius = 0.1f;
        float maxRadius = 1.0f;
        auto spherePrimPtr = makePrimitiveRef(&spherePrimitive);
        for (int i = 0; i < 30; i++)
        {
            Renderer::Matrix4 sphereModel = Renderer::Matrix4::identity();
            float radius = Renderer::Random::randomFloat(minRadius, maxRadius);
            sphereModel.scaleBy(radius, radius, radius);
            sphereModel.translate(Renderer::Random::randomFloat(minX, maxX), Renderer::Random::randomFloat(minY, maxY),
                                  Renderer::Random::randomFloat(minZ, maxZ));
            sphereModel = sphereModel.transpose();
            auto renderable = std::make_shared<Renderer::Renderable>();
            renderable->setPrimitive(spherePrimPtr);
            renderable->setTransform(sphereModel);
            renderable->setColor(Renderer::Random::randomColor());
            scene->AddRenderable(renderable);
            sphereRenderables.push_back(renderable);
        }
        // 轻量光源球体
        auto lightSphereRenderable = std::make_shared<Renderer::Renderable>();
        lightSphereRenderable->setPrimitive(spherePrimPtr);
        lightSphereRenderable->setColor(Renderer::Vector3(1.0f, 1.0f, 1.0f));
        scene->AddRenderable(lightSphereRenderable);
        // 特效半透明球体示例（正向渲染，不进入延迟管线）
        Renderer::Matrix4 fxSphereModel = Renderer::Matrix4::identity();
        // fxSphereModel.scaleBy(1.0f, 1.0f, 1.0f);
        // fxSphereModel.translate(0.0f, 0.0f, 0.0f);
        fxSphereModel = fxSphereModel.transpose();
        auto fxSphereRenderable = std::make_shared<Renderer::Renderable>();
        auto cubePrimPtr = makePrimitiveRef(&cubePrimitive);
        fxSphereRenderable->setPrimitive(cubePrimPtr);
        fxSphereRenderable->setTransform(fxSphereModel);
        fxSphereRenderable->setColor(Renderer::Vector3(0.2f, 0.8f, 1.0f));
        forwardRenderables.push_back(fxSphereRenderable);
        guiLayer.SetSelectBox(fxSphereRenderable);
        // 地面
        auto quadPrimPtr = makePrimitiveRef(&quadPrimitive);
        Renderer::Matrix4 quadModel = Renderer::Matrix4::identity();
        quadModel.scaleBy(10.0f, 10.0f, 10.0f);
        quadModel.rotate(DEG2RAD(-90.0f), Renderer::Vector3(1.0f, 0.0f, 0.0f));
        quadModel = quadModel.transpose();
        auto quadRenderable = std::make_shared<Renderer::Renderable>();
        quadRenderable->setPrimitive(quadPrimPtr);
        quadRenderable->setTransform(quadModel);
        quadRenderable->setColor(Renderer::Vector3(0.5f, 0.5f, 0.5f));
        // scene->AddRenderable(quadRenderable);
        //  模型实例
        Renderer::Matrix4 model1M = Renderer::Matrix4::identity();
        model1M.scaleBy(0.3f, 0.3f, 0.3f);
        model1M.translate(0.0f, 1.0f, 2.0f);
        model1M = model1M.transpose();
        auto model1Renderable = std::make_shared<Renderer::Renderable>();
        model1Renderable->setModel(loadedModel);
        model1Renderable->setTransform(model1M);
        // scene->AddRenderable(model1Renderable);

        Renderer::Matrix4 model2M = Renderer::Matrix4::identity();
        model2M.scaleBy(0.01f, 0.01f, 0.01f);
        model2M.rotate(DEG2RAD(10.0f), Renderer::Vector3(1.0f, 0.0f, 0.0f));
        model2M.translate(-3.0f, -1.6f, 0.0f);
        model2M = model2M.transpose();
        auto model2Renderable = std::make_shared<Renderer::Renderable>();
        model2Renderable->setModel(loadedModel2);
        model2Renderable->setTransform(model2M);
        scene->AddRenderable(model2Renderable);

        float lastTime = 0.0f;
        int frameCount = 0;
        float startTime = Renderer::Window::getTime();
        unsigned int currentSelectedUID = 0;
        std::shared_ptr<Renderer::Renderable> selectedRenderable = nullptr;
        while (!window.shouldClose())
        {
            float currentTime = static_cast<float>(Renderer::Window::getTime());
            float deltaTime = currentTime - lastTime;
            lastTime = currentTime;
            frameCount++;
            if (currentTime - startTime >= 1.0f)
            {
                float fps = static_cast<float>(frameCount) / (currentTime - startTime);
                LOG_INFO("FPS: {}", int(fps));
                startTime = currentTime;
                frameCount = 0;
            }
            window.pollEvents();
            guiLayer.BeginFrame();
            eventBus.Dispatch();

            if (inputState.exitRequested)
                break;

            if (inputState.moveForward)
                camera.processKeyboard(Renderer::CameraMovement::Forward, deltaTime);
            if (inputState.moveBackward)
                camera.processKeyboard(Renderer::CameraMovement::Backward, deltaTime);
            if (inputState.moveLeft)
                camera.processKeyboard(Renderer::CameraMovement::Left, deltaTime);
            if (inputState.moveRight)
                camera.processKeyboard(Renderer::CameraMovement::Right, deltaTime);
            if (inputState.moveUp)
                camera.processKeyboard(Renderer::CameraMovement::Up, deltaTime);
            if (inputState.moveDown)
                camera.processKeyboard(Renderer::CameraMovement::Down, deltaTime);

            bool isDrawPoints = inputState.togglePoints;

            unsigned int mouseXInt = static_cast<unsigned int>(inputState.mouseX);
            unsigned int mouseYInt = WIN_HEIGHT - static_cast<unsigned int>(inputState.mouseY);

            Renderer::Vector3 lightPos(0.0f, 5.0f, 0.0f);
            float curX = 5.0f * std::sin(currentTime);
            float curZ = 5.0f * std::cos(currentTime);
            lightPos.x = curX;
            lightPos.z = curZ;
            // 更新光源球体变换
            Renderer::Matrix4 lightModel = Renderer::Matrix4::identity();
            lightModel.scaleBy(0.1f, 0.1f, 0.1f);
            lightModel.translate(lightPos.x, lightPos.y, lightPos.z);
            lightModel = lightModel.transpose();
            lightSphereRenderable->setTransform(lightModel);

            // 更新视图矩阵
            camera.getViewMatrix(viewMatrix);
            float vx, vy, vz;
            camera.getPosition(vx, vy, vz);
            forwardEffectShader.use();
            forwardEffectShader.setVec3("u_viewPos", vx, vy, vz);
            forwardEffectShader.unuse();

            if (isDrawPoints)
            {
                gaussianRenderer.drawPoints(Renderer::Matrix4::identity(), viewMatrix, projMatrix);
                unsigned int displayTex = gaussianRenderer.getColorTexture();
                finalPass.render(WIN_WIDTH, WIN_HEIGHT, displayTex);
            }
            else
            {
                geometryPass.Begin(viewMatrix, projMatrix);

                for (auto &r : scene->GetRenderables())
                {
                    if (!r)
                        continue;
                    geometryPass.Render(r.get());
                }

                geometryPass.End();

                if (inputState.pickRequested)
                {
                    unsigned int picked = geometryPass.getCurrentSelectedUID(mouseXInt, mouseYInt);
                    inputState.pickRequested = false;
                    if (picked != 0)
                    {
                        currentSelectedUID = picked;
                        selectedRenderable = scene->GetRenderableByUID(picked);
                    }
                    currentSelectedUID = picked;
                }

                lightingPass.Begin(camera, lightPos);
                lightingPass.Render(geometryPass.getPositionTexture(), geometryPass.getNormalTexture(),
                                    geometryPass.getDiffuseTexture(), geometryPass.getSpecularTexture(),
                                    geometryPass.getShininessTexture());
                lightingPass.End();
                // 使用高质量的Splat渲染
                if (gbufferViewMode == static_cast<int>(ViewMode::Gaussian))
                {
                    Renderer::Vector3 selectBoxPos, selectBoxSize;
                    guiLayer.GetSelectBoxPosSize(selectBoxPos, selectBoxSize);
                    Renderer::Vector3 selectColor = guiLayer.GetSelectColor();
                    bool deleteSelectPoints = guiLayer.GetDeleteSelectPoints();
                    gaussianRenderer.drawSplats(Renderer::Matrix4::identity(), viewMatrix, projMatrix, WIN_WIDTH,
                                                WIN_HEIGHT, geometryPass.getDepthTexture(), selectBoxPos, selectBoxSize,
                                                deleteSelectPoints, selectColor, guiLayer.GetGaussianScale());
                }
                // gaussianRenderer.drawSplats(Renderer::Matrix4::identity(), viewMatrix, projMatrix, WIN_WIDTH,
                // WIN_HEIGHT, geometryPass.getDepthTexture());

                postProcessPass.render(WIN_WIDTH, WIN_HEIGHT, camera, currentSelectedUID, geometryPass.getUIDTexture(),
                                       geometryPass.getPositionTexture(), geometryPass.getNormalTexture(),
                                       lightingPass.getLightingTexture(), geometryPass.getDepthTexture(),
                                       gaussianRenderer.getColorTexture());

                if (guiLayer.GetSelectBoxEnabled())
                {
                    // 正向渲染队列（半透明/特效物体）叠加到后处理颜色缓冲
                    forwardPass.Render(WIN_WIDTH, WIN_HEIGHT, viewMatrix, projMatrix, postProcessPass.getColorTexture(),
                                       geometryPass.getDepthTexture(), forwardRenderables, forwardEffectShader,
                                       currentTime);
                }

                unsigned int displayTex = postProcessPass.getColorTexture();
                switch (static_cast<ViewMode>(gbufferViewMode))
                {
                case ViewMode::Final:
                    displayTex = postProcessPass.getColorTexture();
                    break;
                case ViewMode::Lighting:
                    displayTex = lightingPass.getLightingTexture();
                    break;
                case ViewMode::Position:
                    displayTex = geometryPass.getPositionTexture();
                    break;
                case ViewMode::Normal:
                    displayTex = geometryPass.getNormalTexture();
                    break;
                case ViewMode::Diffuse:
                    displayTex = geometryPass.getDiffuseTexture();
                    break;
                case ViewMode::Specular:
                    displayTex = geometryPass.getSpecularTexture();
                    break;
                case ViewMode::Shininess:
                    displayTex = geometryPass.getShininessTexture();
                    break;
                case ViewMode::Depth:
                    displayTex = geometryPass.getDepthTexture();
                    break;
                case ViewMode::Gaussian:
                    displayTex = postProcessPass.getColorTexture(); // gaussianRenderer.getColorTexture();
                    break;
                default:
                    break;
                }
                finalPass.render(WIN_WIDTH, WIN_HEIGHT, displayTex);
                // finalPass.render(WIN_WIDTH, WIN_HEIGHT, lightingPass.getLightingTexture());
                // finalPass.render(WIN_WIDTH, WIN_HEIGHT, geometryPass.getUIDTexture());
            }
            if (selectedRenderable)
            {
                guiLayer.SetSelectedRenderable(selectedRenderable, currentSelectedUID);
            }
            guiLayer.RenderGUI();
            guiLayer.EndFrame();
            window.swapBuffers();
        }

        guiLayer.Shutdown();
        Renderer::Window::terminateGLFW();
        LOG_INFO("程序正常退出");
        return 0;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("发生错误: {}", e.what());
        Renderer::Window::terminateGLFW();
        return -1;
    }
}
