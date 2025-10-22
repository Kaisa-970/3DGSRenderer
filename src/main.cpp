#include <iostream>
#include <cmath>
#include "Renderer/Shader.h"
#include "Renderer/Window.h"
#include "Renderer/RendererContext.h"
#include "Renderer/Camera.h"
#include "Logger/Log.h"

int main() {
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
        Renderer::Window window(800, 600, "3DGS Renderer");
        LOG_INFO("窗口创建成功: 800x600");
        
        // 创建渲染器上下文
        Renderer::RendererContext rendererContext;
        LOG_INFO("渲染器上下文创建成功");

        // 创建相机
        Renderer::Camera camera(0.0f, 0.0f, 3.0f);
        camera.setMovementSpeed(2.5f);
        camera.setMouseSensitivity(0.1f);
        
        float x, y, z;
        camera.getPosition(x, y, z);
        LOG_INFO("相机创建成功，位置: ({}, {}, {})", x, y, z);

        // 创建着色器
#ifdef USE_GLES3
        const char* vertSrc = R"(#version 310 es
precision highp float;
void main(){
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
})";
        const char* fragSrc = R"(#version 310 es
precision mediump float;
out vec4 FragColor;
void main(){
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
})";
#else
        const char* vertSrc = R"(#version 420 core
void main(){
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
})";
        const char* fragSrc = R"(#version 420 core
out vec4 FragColor;
void main(){
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
})";
#endif
        Renderer::Shader shader(vertSrc, fragSrc);
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
        
        while (!window.shouldClose()) {
            
            // 测试：让相机自动旋转（演示）
            static float angle = 0.0f;
            angle += 20.0f * deltaTime; // 每秒旋转 20 度
            
            // 更新相机位置（绕圆圈移动）
            float radius = 3.0f;
            float camX = radius * std::sin(angle * 3.14159f / 180.0f);
            float camZ = radius * std::cos(angle * 3.14159f / 180.0f);
            camera.setPosition(camX, 0.0f, camZ);
            
            // 更新视图矩阵
            camera.getViewMatrix(viewMatrix);
            
            rendererContext.clear(0.2f, 0.3f, 0.3f, 1.0f);
            
            shader.use();
            // 这里可以传递矩阵到着色器
            // 例如: shader.setMat4("view", viewMatrix);
            
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