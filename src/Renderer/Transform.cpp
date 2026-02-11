#include "Transform.h"

RENDERER_NAMESPACE_BEGIN

Transform::Transform() : position(0.0f, 0.0f, 0.0f), rotation(0.0f, 0.0f, 0.0f), scale(1.0f, 1.0f, 1.0f)
{
}

Transform::~Transform()
{
}

Matrix4 Transform::GetMatrix() const
{
    Matrix4 matrix = Matrix4::identity();
    matrix.scale(scale);
    matrix.rotate(-rotation.pitch, Vector3(1.0f, 0.0f, 0.0f));
    matrix.rotate(rotation.yaw, Vector3(0.0f, 1.0f, 0.0f));
    matrix.rotate(rotation.roll, Vector3(0.0f, 0.0f, 1.0f));
    matrix.translate(position);
    matrix = matrix.transpose();
    return matrix;
}

RENDERER_NAMESPACE_END
