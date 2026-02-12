#include "Transform.h"

constexpr float DEG_TO_RAD = 3.1415926f / 180.0f;
#define DEG2RAD(x) (x * DEG_TO_RAD)

RENDERER_NAMESPACE_BEGIN

Transform::Transform() : position(0.0f, 0.0f, 0.0f), rotation(0.0f, 0.0f, 0.0f), scale(1.0f, 1.0f, 1.0f)
{
}

Transform::~Transform()
{
}

Mat4 Transform::GetMatrix() const
{
    Mat4 matrix(1.0f);
    matrix.Translate(position);
    matrix.Rotate(DEG2RAD(rotation.pitch), Vector3(1.0f, 0.0f, 0.0f));
    matrix.Rotate(DEG2RAD(rotation.yaw), Vector3(0.0f, 1.0f, 0.0f));
    matrix.Rotate(DEG2RAD(rotation.roll), Vector3(0.0f, 0.0f, 1.0f));
    matrix.Scale(scale);
    return matrix;
}

RENDERER_NAMESPACE_END
