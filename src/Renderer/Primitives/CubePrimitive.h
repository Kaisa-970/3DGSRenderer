#pragma once

#include "Primitive.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API CubePrimitive : public Primitive {
public:
    explicit CubePrimitive(float size = 1.0f);
    ~CubePrimitive() override = default;

private:
    void generateCube(float size);
};

RENDERER_NAMESPACE_END