#pragma once

#include "Core/RenderCore.h"
#include <string>

struct GLFWwindow;
//class GLFWwindow;
RENDERER_NAMESPACE_BEGIN

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

    static bool initGLFW();
    static void terminateGLFW();

private:
    GLFWwindow* window_ = nullptr;
    int width_;
    int height_;
    std::string title_;
};

RENDERER_NAMESPACE_END