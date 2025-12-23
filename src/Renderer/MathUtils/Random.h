#pragma once

#include "Core/BaseDef.h"
#include "Vector.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API Random
{
public:
    static float randomFloat(float min, float max);
    static int randomInt(int min, int max);
    static Vector3 randomVector3(float min, float max);
    static Vector3 randomColor();
};

RENDERER_NAMESPACE_END
