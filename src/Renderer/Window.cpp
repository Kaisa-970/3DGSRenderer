#include "Window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include "Logger/Log.h"

RENDERER_NAMESPACE_BEGIN

bool Window::initGLFW() {
    if (!glfwInit()) {
        LOG_CORE_ERROR("Failed to initialize GLFW");
        return false;
    }
    return true;
}

void Window::terminateGLFW() {
    glfwTerminate();
}

Window::Window(int width, int height, const std::string& title)
    : width_(width), height_(height), title_(title) {
    
#ifdef USE_GLES3
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
    if (!window_) {
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window_);

    // 初始化 GLAD
#ifdef USE_GLES3
    if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD (OpenGL ES)");
    }
#else
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD (OpenGL)");
    }
#endif

    // 打印 OpenGL 信息
#ifdef USE_GLES3
    std::cout << "=== OpenGL ES 3 Information ===" << std::endl;
#else
    LOG_CORE_INFO("=== OpenGL Information ===");
#endif
    LOG_CORE_INFO("Vendor: {}", (const char*)glGetString(GL_VENDOR));
    LOG_CORE_INFO("Renderer: {}", (const char*)glGetString(GL_RENDERER));
    LOG_CORE_INFO("Version: {}", (const char*)glGetString(GL_VERSION));
    LOG_CORE_INFO("GLSL Version: {}", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    LOG_CORE_INFO("================================");
}

Window::~Window() {
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

void Window::swapBuffers() {
    glfwSwapBuffers(window_);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void* Window::getNativeHandle() const {
    return reinterpret_cast<void*>(window_);
}

RENDERER_NAMESPACE_END