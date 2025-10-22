#pragma once

#include "Core/RenderCore.h"
#include <string>
#include <functional>

struct GLFWwindow;
//class GLFWwindow;
RENDERER_NAMESPACE_BEGIN

// 键盘键码枚举
enum class Key {
    // 字母
    A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72,
    I = 73, J = 74, K = 75, L = 76, M = 77, N = 78, O = 79, P = 80,
    Q = 81, R = 82, S = 83, T = 84, U = 85, V = 86, W = 87, X = 88,
    Y = 89, Z = 90,
    
    // 数字
    Num0 = 48, Num1 = 49, Num2 = 50, Num3 = 51, Num4 = 52,
    Num5 = 53, Num6 = 54, Num7 = 55, Num8 = 56, Num9 = 57,
    
    // 特殊键
    Space = 32,
    Escape = 256,
    Enter = 257,
    Tab = 258,
    Backspace = 259,
    
    // 修饰键
    LeftShift = 340,
    LeftControl = 341,
    LeftAlt = 342,
    RightShift = 344,
    RightControl = 345,
    RightAlt = 346,
    
    // 箭头键
    Right = 262,
    Left = 263,
    Down = 264,
    Up = 265,
    
    // 功能键
    F1 = 290, F2 = 291, F3 = 292, F4 = 293,
    F5 = 294, F6 = 295, F7 = 296, F8 = 297,
    F9 = 298, F10 = 299, F11 = 300, F12 = 301
};

// 鼠标按键枚举
enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2
};

class RENDERER_API Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();
    
    void* getNativeHandle() const;
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

    // 输入查询
    bool isKeyPressed(Key key) const;
    bool isMouseButtonPressed(MouseButton button) const;
    void getMousePosition(double& x, double& y) const;
    void setMousePosition(double x, double y); 
    
    // 光标模式设置
    enum class CursorMode {
        Normal,      // 正常显示光标
        Hidden,      // 隐藏光标
        Disabled     // 禁用光标（FPS模式）
    };
    void setCursorMode(CursorMode mode);
    
    // 回调设置（使用std::function便于绑定）
    using MouseMoveCallback = std::function<void(double xpos, double ypos)>;
    using MouseScrollCallback = std::function<void(double xoffset, double yoffset)>;
    using KeyCallback = std::function<void(int key, int scancode, int action, int mods)>;
    
    void setMouseMoveCallback(MouseMoveCallback callback);
    void setMouseScrollCallback(MouseScrollCallback callback);
    void setKeyCallback(KeyCallback callback);

    static bool initGLFW();
    static void terminateGLFW();
    static double getTime();
private:
    GLFWwindow* window_ = nullptr;
    int width_;
    int height_;
    std::string title_;

     // 回调函数存储
     MouseMoveCallback mouseMoveCallback_;
     MouseScrollCallback mouseScrollCallback_;
     KeyCallback keyCallback_;
     
     // GLFW静态回调（转发到成员函数）
     static void glfwMouseCallback(GLFWwindow* window, double xpos, double ypos);
     static void glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
     static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

RENDERER_NAMESPACE_END