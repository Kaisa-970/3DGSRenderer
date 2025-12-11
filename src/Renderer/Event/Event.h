#pragma once

#include "Core/RenderCore.h"

RENDERER_NAMESPACE_BEGIN

enum class EventType {
    Key,
    MouseButton,
    MouseMove,
    Scroll,
    WindowResize
};

struct Event {
    EventType type;
    bool handled{false};
    virtual ~Event() = default;
};

struct KeyEvent : public Event {
    int key{0};
    int scancode{0};
    int action{0};
    int mods{0};
    double time{0.0};

    KeyEvent(int k, int sc, int act, int m, double t) {
        type = EventType::Key;
        key = k;
        scancode = sc;
        action = act;
        mods = m;
        time = t;
    }
};

struct MouseButtonEvent : public Event {
    int button{0};
    int action{0};
    int mods{0};
    double x{0.0};
    double y{0.0};
    double time{0.0};

    MouseButtonEvent(int btn, int act, int m, double px, double py, double t) {
        type = EventType::MouseButton;
        button = btn;
        action = act;
        mods = m;
        x = px;
        y = py;
        time = t;
    }
};

struct MouseMoveEvent : public Event {
    double x{0.0};
    double y{0.0};
    double dx{0.0};
    double dy{0.0};
    double time{0.0};

    MouseMoveEvent(double px, double py, double pdx, double pdy, double t) {
        type = EventType::MouseMove;
        x = px;
        y = py;
        dx = pdx;
        dy = pdy;
        time = t;
    }
};

struct ScrollEvent : public Event {
    double xoffset{0.0};
    double yoffset{0.0};
    double time{0.0};

    ScrollEvent(double xo, double yo, double t) {
        type = EventType::Scroll;
        xoffset = xo;
        yoffset = yo;
        time = t;
    }
};

struct WindowResizeEvent : public Event {
    int width{0};
    int height{0};

    WindowResizeEvent(int w, int h) {
        type = EventType::WindowResize;
        width = w;
        height = h;
    }
};

RENDERER_NAMESPACE_END

