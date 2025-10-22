#pragma once

#include "Primitive.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API CubePrimitive : public Primitive {
public:
    explicit CubePrimitive(float size = 1.0f, bool colored = true);
    ~CubePrimitive() override = default;

private:
    void generateCube(float size, bool colored);
};

RENDERER_NAMESPACE_END