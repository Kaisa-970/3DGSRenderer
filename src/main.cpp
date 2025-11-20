#include <iostream>
#include <cmath>
#include "Renderer/Shader.h"
#include "Renderer/Window.h"
#include "Renderer/RendererContext.h"
#include "Renderer/Camera.h"
#include "Logger/Log.h"
#include "Renderer/Primitives/CubePrimitive.h"
#include "Renderer/Primitives/SpherePrimitive.h"
#include "Renderer/Primitives/QuadPrimitive.h"
#include "Renderer/GaussianSplatting/GaussianRenderer.h"
#include "Renderer/FinalPass.h"
#include "Renderer/GeometryPass.h"
#include "Renderer/LightingPass.h"
#include "Renderer/MathUtils/Random.h"
#include "Renderer/PostProcessPass.h"

#ifdef GSRENDERER_OS_WINDOWS
#include <windows.h>
#endif

// 鼠标状态
struct MouseState {
    bool firstMouse = true;
    bool isRotating = false;
    float lastX = 0.0f;  // 初始值无关紧要，firstMouse 会处理第一次输入
    float lastY = 0.0f;
} mouseState;

std::string modelPath = "res/point_cloud_2.ply";

const int WIN_WIDTH = 1600;
const int WIN_HEIGHT = 900;

int main(int argc, char* argv[]) {
#ifdef GSRENDERER_OS_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 初始化日志系统
    Logger::Log::Init();
    
    LOG_INFO("3DGS Renderer 启动中...");
    
    try {
        // 初始化 GLFW
        if (!Renderer::Window::initGLFW()) {
            LOG_ERROR("初始化 GLFW 失败");
            return -1;
        }
        LOG_INFO("GLFW 初始化成功");

        // 创建窗口（自动初始化 OpenGL 上下文）
        Renderer::Window window(WIN_WIDTH, WIN_HEIGHT, "3DGS Renderer");
        LOG_INFO("窗口创建成功: {}x{}", WIN_WIDTH, WIN_HEIGHT);
        
        // 创建渲染器上下文
        Renderer::RendererContext rendererContext;
        LOG_INFO("渲染器上下文创建成功");

        if (argc > 1) 
        {
            modelPath = std::string(argv[1]);
            LOG_INFO("使用模型文件: {}", modelPath);
        }
        
        Renderer::GaussianRenderer gaussianRenderer;
        //gaussianRenderer.loadModel("res/input.ply");
        gaussianRenderer.loadModel(modelPath);
        

        // 创建相机（根据模型自动计算位置）
        // 模型中心约在 (-4.39, -4.85, -3.90)，尺寸约 48.50
        // 将相机放在模型前方，距离约为尺寸的1.5倍
        Renderer::Vector3 camPos(4.42f, -1.0f, -3.63f);
        Renderer::Camera camera(
            camPos,  // 位置（模型中心前方）
            Renderer::Vector3(0.0f, -1.0f, 0.0f),        // 世界上方向
            133.5f,  // yaw: -90度 朝向 -Z 方向（看向模型）
            -14.0f     // pitch: 0度 水平视角
        );
        camera.setMovementSpeed(2.0f);  // 增大移动速度，因为场景较大
        camera.setMouseSensitivity(0.1f);
        
        float x, y, z;
        camera.getPosition(x, y, z);
        LOG_INFO("相机创建成功，位置: ({}, {}, {})", x, y, z);

        // 设置鼠标移动回调
        // 注意：在 GLFW_CURSOR_DISABLED 模式下，xpos/ypos 是虚拟坐标，会持续累加
        window.setMouseMoveCallback([&camera, &window](double xpos, double ypos) {
            if (!window.isMouseButtonPressed(Renderer::MouseButton::Right)) {
                mouseState.firstMouse = true;  // 重置状态
                return;
            }

            if (mouseState.firstMouse) {
                mouseState.lastX = static_cast<float>(xpos);
                mouseState.lastY = static_cast<float>(ypos);
                mouseState.firstMouse = false;
                return;  // 跳过第一次输入，避免初始跳跃
            }

            float xoffset = static_cast<float>(xpos) - mouseState.lastX;
            float yoffset = mouseState.lastY - static_cast<float>(ypos);
            
            // 防止窗口焦点切换时产生的巨大跳跃（阈值：100像素）
            if (std::abs(xoffset) > 100.0f || std::abs(yoffset) > 100.0f) {
                mouseState.lastX = static_cast<float>(xpos);
                mouseState.lastY = static_cast<float>(ypos);
                return;  // 忽略异常大的偏移
            }
            
            mouseState.lastX = static_cast<float>(xpos);
            mouseState.lastY = static_cast<float>(ypos);

            camera.processMouseMovement(xoffset, yoffset);
        });

        // 设置鼠标滚轮回调
        window.setMouseScrollCallback([&camera](double xoffset, double yoffset) {
            camera.processMouseScroll(static_cast<float>(yoffset));
        });

        // 先设置光标模式为禁用（FPS模式），这会锁定并隐藏鼠标
        //window.setCursorMode(Renderer::Window::CursorMode::Disabled);

        // 创建彩色立方体
        Renderer::CubePrimitive cubePrimitive(1.0f, false);

        Renderer::SpherePrimitive spherePrimitive(1.0f, 64, 32, false);

        Renderer::QuadPrimitive quadPrimitive(10.0f, false);

        Renderer::Shader shader = Renderer::Shader::fromFiles(
            "res/shaders/cube.vs.glsl", 
            "res/shaders/cube.fs.glsl");

        // std::vector<Renderer::SpherePrimitive*> spherePrimitives;
        // for (int i = 0; i < 10; i++)
        // {
        //     spherePrimitives.push_back(new Renderer::SpherePrimitive(1.0f, 64, 32, false));
        // }
        // 测试相机矩阵
        float viewMatrix[16];
        float projMatrix[16];
        camera.getViewMatrix(viewMatrix);
        camera.getPerspectiveMatrix(projMatrix, 50.0f, static_cast<float>(WIN_WIDTH) / static_cast<float>(WIN_HEIGHT), 0.01f, 1000.0f);

        // 渲染循环
        float deltaTime = 0.016f; // 简化版：假设 60 FPS
        
        LOG_INFO("=== 相机控制 ===");
        LOG_INFO("这是一个基本的相机测试");
        LOG_INFO("按 Ctrl+C 退出");
        LOG_INFO("===================");
        
        Renderer::Shader lambertShader = Renderer::Shader::fromFiles(
            "res/shaders/lambert.vs.glsl", 
            "res/shaders/lambert.fs.glsl"
        );

        Renderer::FinalPass finalPass;
        Renderer::GeometryPass geometryPass;
        Renderer::LightingPass lightingPass;
        Renderer::PostProcessPass postProcessPass;
        std::vector<Renderer::Matrix4> sphereModels;
        std::vector<Renderer::Vector3> sphereColors;
        float minX = -10.0f;
        float maxX = 10.0f;
        float minY = -10.0f;
        float maxY = 10.0f;
        float minZ = -10.0f;
        float maxZ = 10.0f;
        float minRadius = 0.1f;
        float maxRadius = 1.0f;
        for (int i = 0; i < 30; i++)
        {
            Renderer::Matrix4 sphereModel = Renderer::Matrix4::identity();
            float radius = Renderer::Random::randomFloat(minRadius, maxRadius);
            sphereModel.scaleBy(radius, radius, radius);
            sphereModel.translate(Renderer::Random::randomFloat(minX, maxX), Renderer::Random::randomFloat(minY, maxY), Renderer::Random::randomFloat(minZ, maxZ));
            sphereModel = sphereModel.transpose();
            sphereModels.push_back(sphereModel);

            sphereColors.push_back(Renderer::Random::randomColor());
        }

        float lastTime = 0.0f;
        bool isDrawPoints = false;
        while (!window.shouldClose()) {
            float currentTime = static_cast<float>(Renderer::Window::getTime());
            float deltaTime = currentTime - lastTime;
            lastTime = currentTime;
            {
                // 处理键盘输入
                if (window.isKeyPressed(Renderer::Key::Escape))
                    break;
                
                if (window.isKeyPressed(Renderer::Key::W))
                    camera.processKeyboard(Renderer::CameraMovement::Forward, deltaTime);
                if (window.isKeyPressed(Renderer::Key::S))
                    camera.processKeyboard(Renderer::CameraMovement::Backward, deltaTime);
                if (window.isKeyPressed(Renderer::Key::A))
                    camera.processKeyboard(Renderer::CameraMovement::Left, deltaTime);
                if (window.isKeyPressed(Renderer::Key::D))
                    camera.processKeyboard(Renderer::CameraMovement::Right, deltaTime);
                if (window.isKeyPressed(Renderer::Key::E))
                    camera.processKeyboard(Renderer::CameraMovement::Up, deltaTime);
                if (window.isKeyPressed(Renderer::Key::Q))
                    camera.processKeyboard(Renderer::CameraMovement::Down, deltaTime);
            
                isDrawPoints = false;
                if (window.isKeyPressed(Renderer::Key::P))
                {
                    isDrawPoints = true;//!isDrawPoints;
                    LOG_ERROR("切换点云渲染模式: {}", isDrawPoints ? "Points" : "Splats");
                }
            }

            Renderer::Vector3 lightPos(0.0f, -5.0f, 0.0f);
            float curX = 5.0f * std::sin(currentTime);
            float curZ = 5.0f * std::cos(currentTime);
            lightPos.x = curX;
            lightPos.z = curZ;

            // 更新视图矩阵
            camera.getViewMatrix(viewMatrix);
            
            rendererContext.clear(0.2f, 0.3f, 0.3f, 1.0f);
            rendererContext.clear(0.0f, 0.0f, 0.0f, 1.0f);
            
            if (isDrawPoints)
            {
                gaussianRenderer.drawPoints(Renderer::Matrix4::identity(), viewMatrix, projMatrix);
                finalPass.render(WIN_WIDTH, WIN_HEIGHT, gaussianRenderer.getColorTexture());
            }
            else
            {
                // // 使用高质量的Splat渲染
                //gaussianRenderer.drawSplats(Renderer::Matrix4::identity(), viewMatrix, projMatrix, 1600, 900);
                
                Renderer::Matrix4 model = Renderer::Matrix4::identity();

                model.scaleBy(0.1f, 0.1f, 0.1f);
                model.translate(lightPos.x, lightPos.y, lightPos.z);
                model = model.transpose();
                geometryPass.clear();

                for (int i = 0; i < sphereModels.size(); i++)
                {
                    geometryPass.render(&spherePrimitive, sphereModels[i].m, viewMatrix, projMatrix, sphereColors[i]);
                }
                geometryPass.render(&spherePrimitive, model.m, viewMatrix, projMatrix, Renderer::Vector3(1.0f, 1.0f, 1.0f));
                
                model = Renderer::Matrix4::identity();
                geometryPass.render(&cubePrimitive, model.m, viewMatrix, projMatrix, Renderer::Vector3(1.0f, 0.0f, 0.0f));
                
                model = Renderer::Matrix4::identity();
                model.scaleBy(10.0f, 10.0f, 10.0f);
                model.rotate(-90.0f * 3.1415926f / 180.0f, Renderer::Vector3(1.0f, 0.0f, 0.0f));
                geometryPass.render(&quadPrimitive, model.m, viewMatrix, projMatrix, Renderer::Vector3(0.5f, 0.5f, 0.5f));
                
                lightingPass.render(WIN_WIDTH, WIN_HEIGHT, camera, lightPos, geometryPass.getPositionTexture(), geometryPass.getNormalTexture(), geometryPass.getColorTexture());
                
                postProcessPass.render(WIN_WIDTH, WIN_HEIGHT, camera, geometryPass.getPositionTexture(), geometryPass.getNormalTexture(), lightingPass.getLightingTexture(), geometryPass.getDepthTexture());
                finalPass.render(WIN_WIDTH, WIN_HEIGHT, postProcessPass.getColorTexture());
                //finalPass.render(WIN_WIDTH, WIN_HEIGHT, gaussianRenderer.getColorTexture());
            }
            window.swapBuffers();
            window.pollEvents();
        }

        Renderer::Window::terminateGLFW();
        LOG_INFO("程序正常退出");
        return 0;

    } catch (const std::exception& e) {
        LOG_ERROR("发生错误: {}", e.what());
        Renderer::Window::terminateGLFW();
        return -1;
    }
}