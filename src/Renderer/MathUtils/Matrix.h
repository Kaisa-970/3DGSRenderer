#pragma once

#include "Core/TypeDef.h"
#include "Vector.h"
#include <glm/glm.hpp>

RENDERER_NAMESPACE_BEGIN

class RENDERER_API Mat4
{
private:
    glm::mat4 m_mat;

public:
    Mat4();
    Mat4(float value);
    Mat4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21,
         float m22, float m23, float m30, float m31, float m32, float m33);
    Mat4(const Mat4 &other);
    ~Mat4();
    Mat4 &operator=(const Mat4 &other);
    Mat4 operator*(const Mat4 &other) const;
    Mat4 operator+(const Mat4 &other) const;
    Mat4 operator-(const Mat4 &other) const;
    Mat4 operator*(float scalar) const;
    Mat4 operator/(float scalar) const;
    Vector3 operator*(const Vector3 &vector) const;
    Mat4 operator-() const;
    bool operator==(const Mat4 &other) const;
    bool operator!=(const Mat4 &other) const;
    float operator()(int col, int row) const;
    float &operator()(int col, int row);
    const float *data() const;
    Mat4 &Translate(const Vector3 &translation);
    Mat4 &Translate(float tx, float ty, float tz);
    Mat4 &Scale(const Vector3 &scale);
    Mat4 &Scale(float sx, float sy, float sz);
    // angle in radians
    Mat4 &Rotate(float angle, const Vector3 &axis);
    Mat4 &Rotate(float angle, float ax, float ay, float az);
    Mat4 &Transpose();
    Mat4 Transposed() const;
    Mat4 &Inverse();
    Mat4 Inversed() const;

    static Mat4 Identity();
    static Mat4 Zero();
    static Mat4 Ortho(float left, float right, float bottom, float top, float near, float far);
    static Mat4 Perspective(float fov, float aspect, float near, float far);
    static Mat4 LookAt(const Vector3 &eye, const Vector3 &center, const Vector3 &up);
    static Mat4 Rotation(float angle, const Vector3 &axis);
    static Mat4 Scaling(const Vector3 &scale);
    static Mat4 Translation(const Vector3 &translation);
};

RENDERER_NAMESPACE_END
