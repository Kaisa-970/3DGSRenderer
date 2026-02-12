#pragma once

#include "Core/TypeDef.h"
#include <glm/glm.hpp>

RENDERER_NAMESPACE_BEGIN

using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;

const Vector3 VECTOR3_ZERO = Vector3(0.0f, 0.0f, 0.0f);
const Vector3 VECTOR3_ONE = Vector3(1.0f, 1.0f, 1.0f);

class RENDERER_API VectorUtils
{
public:
    static Vector3 Normalize(const Vector3 &vector);
    static Vector3 Cross(const Vector3 &a, const Vector3 &b);
    static float Dot(const Vector3 &a, const Vector3 &b);
    static float Distance(const Vector3 &a, const Vector3 &b);
};

RENDERER_NAMESPACE_END
