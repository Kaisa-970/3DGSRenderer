#pragma once

#include "Core/RenderCore.h"
#include "MathUtils/Vector.h"
#include "MathUtils/Matrix.h"

RENDERER_NAMESPACE_BEGIN

struct RENDERER_API Rotator
{
    Rotator() : pitch(0.0f), yaw(0.0f), roll(0.0f)
    {
    }
    Rotator(float pitch, float yaw, float roll) : pitch(pitch), yaw(yaw), roll(roll)
    {
    }
    float pitch;
    float yaw;
    float roll;
};

class RENDERER_API Transform
{
public:
    Transform();
    ~Transform();

    Mat4 GetMatrix() const;

public:
    Vector3 position;
    Rotator rotation;
    Vector3 scale;
};

RENDERER_NAMESPACE_END
