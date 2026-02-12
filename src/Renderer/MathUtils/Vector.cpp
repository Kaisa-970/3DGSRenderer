#include "Vector.h"

RENDERER_NAMESPACE_BEGIN

Vector3 VectorUtils::Normalize(const Vector3 &vector)
{
    return glm::normalize(vector);
}

Vector3 VectorUtils::Cross(const Vector3 &a, const Vector3 &b)
{
    return glm::cross(a, b);
}
float VectorUtils::Dot(const Vector3 &a, const Vector3 &b)
{
    return glm::dot(a, b);
}
float VectorUtils::Distance(const Vector3 &a, const Vector3 &b)
{
    return glm::distance(a, b);
}
RENDERER_NAMESPACE_END
