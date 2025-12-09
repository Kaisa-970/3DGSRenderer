#pragma once

#include "Primitive.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API QuadPrimitive : public Primitive {
public:
    // size: 四边形的尺寸（边长）
    explicit QuadPrimitive(float size = 1.0f);
    ~QuadPrimitive() override = default;

private:
    void generateQuad(float size);
};

RENDERER_NAMESPACE_END

