#include "Matrix.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

RENDERER_NAMESPACE_BEGIN

Mat4::Mat4() : m_mat(1.0f)
{
}

Mat4::Mat4(float value) : m_mat(value)
{
}

Mat4::Mat4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21,
           float m22, float m23, float m30, float m31, float m32, float m33)
    : m_mat(m00, m10, m20, m30, m01, m11, m21, m31, m02, m12, m22, m32, m03, m13, m23, m33)
{
}

Mat4::Mat4(const Mat4 &other) : m_mat(other.m_mat)
{
}

Mat4::~Mat4()
{
}

//---------------------------------
// 运算符重载
//---------------------------------
Mat4 &Mat4::operator=(const Mat4 &other)
{
    if (this != &other)
    {
        m_mat = other.m_mat;
    }
    return *this;
}

Mat4 Mat4::operator*(const Mat4 &other) const
{
    Mat4 result;
    result.m_mat = m_mat * other.m_mat;
    return result;
}

Mat4 Mat4::operator+(const Mat4 &other) const
{
    Mat4 result;
    result.m_mat = m_mat + other.m_mat;
    return result;
}
Mat4 Mat4::operator-(const Mat4 &other) const
{
    Mat4 result;
    result.m_mat = m_mat - other.m_mat;
    return result;
}
Mat4 Mat4::operator*(float scalar) const
{
    Mat4 result;
    result.m_mat = m_mat * scalar;
    return result;
}
Mat4 Mat4::operator/(float scalar) const
{
    Mat4 result;
    result.m_mat = m_mat / scalar;
    return result;
}
Vector3 Mat4::operator*(const Vector3 &vector) const
{
    return glm::vec3(m_mat * glm::vec4(vector.x, vector.y, vector.z, 1.0f));
}
Mat4 Mat4::operator-() const
{
    Mat4 result;
    result.m_mat = -m_mat;
    return result;
}
bool Mat4::operator==(const Mat4 &other) const
{
    return m_mat == other.m_mat;
}
bool Mat4::operator!=(const Mat4 &other) const
{
    return m_mat != other.m_mat;
}
float Mat4::operator()(int col, int row) const
{
    return m_mat[col][row];
}
float &Mat4::operator()(int col, int row)
{
    return m_mat[col][row];
}
//---------------------------------
// 获取矩阵数据
//---------------------------------
const float *Mat4::data() const
{
    return glm::value_ptr(m_mat);
}

//---------------------------------
Mat4 &Mat4::Translate(const Vector3 &translation)
{
    m_mat = glm::translate(m_mat, translation);
    return *this;
}
Mat4 &Mat4::Translate(float tx, float ty, float tz)
{
    return Translate(Vector3(tx, ty, tz));
}
Mat4 &Mat4::Scale(const Vector3 &scale)
{
    m_mat = glm::scale(m_mat, scale);
    return *this;
}
Mat4 &Mat4::Scale(float sx, float sy, float sz)
{
    return Scale(Vector3(sx, sy, sz));
}
Mat4 &Mat4::Rotate(float angle, const Vector3 &axis)
{
    m_mat = glm::rotate(m_mat, angle, axis);
    return *this;
}
Mat4 &Mat4::Rotate(float angle, float ax, float ay, float az)
{
    return Rotate(angle, Vector3(ax, ay, az));
}
Mat4 &Mat4::Transpose()
{
    m_mat = glm::transpose(m_mat);
    return *this;
}
Mat4 Mat4::Transposed() const
{
    Mat4 result;
    result.m_mat = glm::transpose(m_mat);
    return result;
}
Mat4 &Mat4::Inverse()
{
    m_mat = glm::inverse(m_mat);
    return *this;
}
Mat4 Mat4::Inversed() const
{
    Mat4 result;
    result.m_mat = glm::inverse(m_mat);
    return result;
}

//---------------------------------
// 静态工厂方法
//---------------------------------
Mat4 Mat4::Identity()
{
    return Mat4(1.0f);
}
Mat4 Mat4::Zero()
{
    return Mat4(0.0f);
}
Mat4 Mat4::Rotation(float angle, const Vector3 &axis)
{
    Mat4 result;
    result.m_mat = glm::rotate(glm::mat4(1.0f), angle, axis);
    return result;
}
Mat4 Mat4::Scaling(const Vector3 &scale)
{
    Mat4 result;
    result.m_mat = glm::scale(glm::mat4(1.0f), scale);
    return result;
}
Mat4 Mat4::Translation(const Vector3 &translation)
{
    Mat4 result;
    result.m_mat = glm::translate(glm::mat4(1.0f), translation);
    return result;
}

Mat4 Mat4::Ortho(float left, float right, float bottom, float top, float near, float far)
{
    Mat4 result;
    result.m_mat = glm::ortho(left, right, bottom, top, near, far);
    return result;
}
Mat4 Mat4::Perspective(float fov, float aspect, float near, float far)
{
    Mat4 result;
    result.m_mat = glm::perspective(fov, aspect, near, far);
    return result;
}
Mat4 Mat4::LookAt(const Vector3 &eye, const Vector3 &center, const Vector3 &up)
{
    Mat4 result;
    result.m_mat = glm::lookAt(eye, center, up);
    return result;
}
RENDERER_NAMESPACE_END
