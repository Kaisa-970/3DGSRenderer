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


int main() {
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
        Renderer::Window window(1600, 1000, "3DGS Renderer");
        LOG_INFO("窗口创建成功: 1600x1000");
        
        // 创建渲染器上下文
        Renderer::RendererContext rendererContext;
        LOG_INFO("渲染器上下文创建成功");

        
        Renderer::GaussianRenderer gaussianRenderer;
        //gaussianRenderer.loadModel("res/input.ply");
        gaussianRenderer.loadModel("res/point_cloud.ply");
        

        // 创建相机（根据模型自动计算位置）
        // 模型中心约在 (-4.39, -4.85, -3.90)，尺寸约 48.50
        // 将相机放在模型前方，距离约为尺寸的1.5倍
        Renderer::Camera camera(
            Renderer::Vector3(0.0f, 0.0f, 5.0f),  // 位置（模型中心前方）
            Renderer::Vector3(0.0f, 1.0f, 0.0f),        // 世界上方向
            -90.0f,  // yaw: -90度 朝向 -Z 方向（看向模型）
            0.0f     // pitch: 0度 水平视角
        );
        camera.setMovementSpeed(5.0f);  // 增大移动速度，因为场景较大
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
        LOG_INFO("光标模式已设置为 Disabled（FPS 模式）");
        LOG_INFO("如果鼠标仍然可以移出窗口，请检查系统窗口管理器设置");

        // 创建彩色立方体
        Renderer::CubePrimitive cubePrimitive(1.0f, false);
        LOG_INFO("立方体创建成功");

        Renderer::SpherePrimitive spherePrimitive(1.0f, 32, 16, false);

        Renderer::QuadPrimitive quadPrimitive(1.0f, true);

        Renderer::Shader shader = Renderer::Shader::fromFiles(
            "res/shaders/cube.vs.glsl", 
            "res/shaders/cube.fs.glsl");
        LOG_INFO("着色器编译成功");

        // 测试相机矩阵
        float viewMatrix[16];
        float projMatrix[16];
        camera.getViewMatrix(viewMatrix);
        camera.getPerspectiveMatrix(projMatrix, 45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
        
        LOG_INFO("视图矩阵（前4个元素）: {}, {}, {}, {}", 
                     viewMatrix[0], viewMatrix[1], viewMatrix[2], viewMatrix[3]);

        // 渲染循环
        float deltaTime = 0.016f; // 简化版：假设 60 FPS
        
        LOG_INFO("=== 相机控制 ===");
        LOG_INFO("这是一个基本的相机测试");
        LOG_INFO("相机初始位置 (0, 0, 3)");
        LOG_INFO("按 Ctrl+C 退出");
        LOG_INFO("===================");
        
        Renderer::Shader lambertShader = Renderer::Shader::fromFiles(
            "res/shaders/lambert.vs.glsl", 
            "res/shaders/lambert.fs.glsl"
        );

        float lastTime = 0.0f;
        bool isDrawPoints = false;
        while (!window.shouldClose()) {
            
            // // 测试：让相机自动旋转（演示）
            // static float angle = 0.0f;
            // angle += 20.0f * deltaTime; // 每秒旋转 20 度
            
            // // 更新相机位置（绕圆圈移动）
            // float radius = 3.0f;
            // float camX = radius * std::sin(angle * 3.14159f / 180.0f);
            // float camZ = radius * std::cos(angle * 3.14159f / 180.0f);
            // camera.setPosition(camX, 0.0f, camZ);
            // camera.lookAt(0.0f, 0.0f, 0.0f);

            {
                float currentTime = static_cast<float>(Renderer::Window::getTime());
                float deltaTime = currentTime - lastTime;
                lastTime = currentTime;
                
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

            // 更新视图矩阵
            camera.getViewMatrix(viewMatrix);
            
            rendererContext.clear(0.2f, 0.3f, 0.3f, 1.0f);
            
            // shader.use();
            // // 这里可以传递矩阵到着色器
            // // 例如: shader.setMat4("view", viewMatrix);
            // shader.setMat4("model", Renderer::Matrix4::identity().m);
            // shader.setMat4("view", viewMatrix);
            // shader.setMat4("projection", projMatrix);
            
            // cubePrimitive.draw(shader);
            
            if (isDrawPoints)
            {
                lambertShader.use();
                lambertShader.setMat4("view", viewMatrix);
                lambertShader.setMat4("projection", projMatrix);
                lambertShader.setMat4("model", Renderer::Matrix4::identity().m);

                lambertShader.setVec3("lightPos", 1.2f, 1.0f, 2.0f);       // 光源位置
                lambertShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);     // 白光
                lambertShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);   // 物体颜色
                lambertShader.setVec3("viewPos", camera.getPosition().x, camera.getPosition().y, camera.getPosition().z);     // 相机位置

                // 设置光照强度（可选，有默认值）
                lambertShader.setFloat("ambientStrength", 0.1f);    // 环境光强度
                lambertShader.setFloat("specularStrength", 0.5f);   // 镜面反射强度
                lambertShader.setInt("shininess", 32);    

                //cubePrimitive.draw(lambertShader);
                //spherePrimitive.draw(lambertShader);
                //quadPrimitive.draw(lambertShader);
                gaussianRenderer.drawPoints(Renderer::Matrix4::identity(), viewMatrix, projMatrix);
            }
            else
            {
                // 使用高质量的Splat渲染
                gaussianRenderer.drawSplats(Renderer::Matrix4::identity(), viewMatrix, projMatrix, 1600, 1000);
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