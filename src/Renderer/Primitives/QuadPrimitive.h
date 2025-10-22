#pragma once

#include "Primitive.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API QuadPrimitive : public Primitive {
public:
    // size: 四边形的尺寸（边长）
    // colored: 是否使用彩色（四个顶点不同颜色）
    explicit QuadPrimitive(float size = 1.0f, bool colored = false);
    ~QuadPrimitive() override = default;

private:
    void generateQuad(float size, bool colored);
};

RENDERER_NAMESPACE_END

