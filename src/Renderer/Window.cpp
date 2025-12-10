#include "Window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
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

double Window::getTime() {
    return glfwGetTime();
}

Window::Window(int width, int height, const std::string& title)
    : width_(width), height_(height), title_(title) {
    
#ifdef USE_GLES3
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
    if (!window_) {
        throw std::runtime_error("Failed to create GLFW window");
    }

    //glfwSwapInterval(0);
    glfwMakeContextCurrent(window_);

    // 设置window的user pointer为this，用于回调
    glfwSetWindowUserPointer(window_, this);

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

// 输入查询实现
bool Window::isKeyPressed(Key key) const {
    return glfwGetKey(window_, static_cast<int>(key)) == GLFW_PRESS;
}

bool Window::isMouseButtonPressed(MouseButton button) const {
    return glfwGetMouseButton(window_, static_cast<int>(button)) == GLFW_PRESS;
}

void Window::getMousePosition(double& x, double& y) const {
    glfwGetCursorPos(window_, &x, &y);
}

void Window::setMousePosition(double x, double y) {
    glfwSetCursorPos(window_, x, y);
}

// 光标模式设置
void Window::setCursorMode(CursorMode mode) {
    int glfwMode;
    switch (mode) {
        case CursorMode::Normal:
            glfwMode = GLFW_CURSOR_NORMAL;
            break;
        case CursorMode::Hidden:
            glfwMode = GLFW_CURSOR_HIDDEN;
            break;
        case CursorMode::Disabled:
            glfwMode = GLFW_CURSOR_DISABLED;
            break;
        default:
            glfwMode = GLFW_CURSOR_NORMAL;
    }
    glfwSetInputMode(window_, GLFW_CURSOR, glfwMode);
}

// 设置回调
void Window::setMouseMoveCallback(MouseMoveCallback callback) {
    mouseMoveCallback_ = callback;
    if (callback) {
        glfwSetCursorPosCallback(window_, glfwMouseCallback);
    } else {
        glfwSetCursorPosCallback(window_, nullptr);
    }
}

void Window::setMouseScrollCallback(MouseScrollCallback callback) {
    mouseScrollCallback_ = callback;
    if (callback) {
        glfwSetScrollCallback(window_, glfwScrollCallback);
    } else {
        glfwSetScrollCallback(window_, nullptr);
    }
}

void Window::setKeyCallback(KeyCallback callback) {
    keyCallback_ = callback;
    if (callback) {
        glfwSetKeyCallback(window_, glfwKeyCallback);
    } else {
        glfwSetKeyCallback(window_, nullptr);
    }
}

// GLFW静态回调实现（转发到成员函数）
void Window::glfwMouseCallback(GLFWwindow* window, double xpos, double ypos) {
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->mouseMoveCallback_) {
        win->mouseMoveCallback_(xpos, ypos);
    }
}

void Window::glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->mouseScrollCallback_) {
        win->mouseScrollCallback_(xoffset, yoffset);
    }
}

void Window::glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->keyCallback_) {
        win->keyCallback_(key, scancode, action, mods);
    }
}

RENDERER_NAMESPACE_END