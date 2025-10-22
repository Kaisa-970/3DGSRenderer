#include "Matrix.h"
#include <cmath>
#include <cstring>
#include <stdexcept>

RENDERER_NAMESPACE_BEGIN

// 构造函数
Matrix4::Matrix4() {
    setIdentity();
}

Matrix4::Matrix4(FLOAT diagonal) {
    setZero();
    m[0] = m[5] = m[10] = m[15] = diagonal;
}

Matrix4::Matrix4(const FLOAT* data) {
    std::memcpy(m, data, 16 * sizeof(FLOAT));
}

Matrix4::Matrix4(FLOAT m00, FLOAT m01, FLOAT m02, FLOAT m03,
                 FLOAT m10, FLOAT m11, FLOAT m12, FLOAT m13,
                 FLOAT m20, FLOAT m21, FLOAT m22, FLOAT m23,
                 FLOAT m30, FLOAT m31, FLOAT m32, FLOAT m33) {
    m[0] = m00;  m[1] = m01;  m[2] = m02;   m[3] = m03;
    m[4] = m10;  m[5] = m11;  m[6] = m12;   m[7] = m13;
    m[8] = m20;  m[9] = m21;  m[10] = m22;  m[11] = m23;
    m[12] = m30; m[13] = m31; m[14] = m32;  m[15] = m33;
}

Matrix4::~Matrix4() {
}

// 访问元素
FLOAT& Matrix4::operator()(int row, int col) {
    return m[row * 4 + col];
}

const FLOAT& Matrix4::operator()(int row, int col) const {
    return m[row * 4 + col];
}

FLOAT& Matrix4::operator[](int index) {
    if (index < 0 || index >= 16) {
        throw std::out_of_range("Matrix4 index out of range");
    }
    return m[index];
}

const FLOAT& Matrix4::operator[](int index) const {
    if (index < 0 || index >= 16) {
        throw std::out_of_range("Matrix4 index out of range");
    }
    return m[index];
}

// 矩阵加法
Matrix4 Matrix4::operator+(const Matrix4& other) const {
    Matrix4 result;
    for (int i = 0; i < 16; i++) {
        result.m[i] = m[i] + other.m[i];
    }
    return result;
}

// 矩阵减法
Matrix4 Matrix4::operator-(const Matrix4& other) const {
    Matrix4 result;
    for (int i = 0; i < 16; i++) {
        result.m[i] = m[i] - other.m[i];
    }
    return result;
}

// 矩阵乘法
Matrix4 Matrix4::operator*(const Matrix4& other) const {
    Matrix4 result;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            FLOAT sum = 0;
            for (int i = 0; i < 4; i++) {
                sum += m[row * 4 + i] * other.m[i * 4 + col];
            }
            result.m[row * 4 + col] = sum;
        }
    }
    return result;
}

// 标量乘法
Matrix4 Matrix4::operator*(FLOAT scalar) const {
    Matrix4 result;
    for (int i = 0; i < 16; i++) {
        result.m[i] = m[i] * scalar;
    }
    return result;
}

// 矩阵 * 向量
Vector3 Matrix4::operator*(const Vector3& vec) const {
    FLOAT x = m[0] * vec.x + m[1] * vec.y + m[2] * vec.z + m[3];
    FLOAT y = m[4] * vec.x + m[5] * vec.y + m[6] * vec.z + m[7];
    FLOAT z = m[8] * vec.x + m[9] * vec.y + m[10] * vec.z + m[11];
    FLOAT w = m[12] * vec.x + m[13] * vec.y + m[14] * vec.z + m[15];
    
    if (std::abs(w) > 1e-6f) {
        return Vector3(x / w, y / w, z / w);
    }
    return Vector3(x, y, z);
}

// 复合赋值运算符
Matrix4& Matrix4::operator+=(const Matrix4& other) {
    for (int i = 0; i < 16; i++) {
        m[i] += other.m[i];
    }
    return *this;
}

Matrix4& Matrix4::operator-=(const Matrix4& other) {
    for (int i = 0; i < 16; i++) {
        m[i] -= other.m[i];
    }
    return *this;
}

Matrix4& Matrix4::operator*=(const Matrix4& other) {
    *this = *this * other;
    return *this;
}

Matrix4& Matrix4::operator*=(FLOAT scalar) {
    for (int i = 0; i < 16; i++) {
        m[i] *= scalar;
    }
    return *this;
}

// 比较运算符
bool Matrix4::operator==(const Matrix4& other) const {
    return equals(other, 1e-6f);
}

bool Matrix4::operator!=(const Matrix4& other) const {
    return !equals(other, 1e-6f);
}

// 转置
Matrix4 Matrix4::transpose() const {
    Matrix4 result;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            result.m[col * 4 + row] = m[row * 4 + col];
        }
    }
    return result;
}

// 行列式
FLOAT Matrix4::determinant() const {
    FLOAT a00 = m[0], a01 = m[1], a02 = m[2], a03 = m[3];
    FLOAT a10 = m[4], a11 = m[5], a12 = m[6], a13 = m[7];
    FLOAT a20 = m[8], a21 = m[9], a22 = m[10], a23 = m[11];
    FLOAT a30 = m[12], a31 = m[13], a32 = m[14], a33 = m[15];

    FLOAT det = 
        a00 * (a11 * (a22 * a33 - a23 * a32) - a12 * (a21 * a33 - a23 * a31) + a13 * (a21 * a32 - a22 * a31)) -
        a01 * (a10 * (a22 * a33 - a23 * a32) - a12 * (a20 * a33 - a23 * a30) + a13 * (a20 * a32 - a22 * a30)) +
        a02 * (a10 * (a21 * a33 - a23 * a31) - a11 * (a20 * a33 - a23 * a30) + a13 * (a20 * a31 - a21 * a30)) -
        a03 * (a10 * (a21 * a32 - a22 * a31) - a11 * (a20 * a32 - a22 * a30) + a12 * (a20 * a31 - a21 * a30));

    return det;
}

// 逆矩阵
Matrix4 Matrix4::inverse() const {
    FLOAT a00 = m[0], a01 = m[1], a02 = m[2], a03 = m[3];
    FLOAT a10 = m[4], a11 = m[5], a12 = m[6], a13 = m[7];
    FLOAT a20 = m[8], a21 = m[9], a22 = m[10], a23 = m[11];
    FLOAT a30 = m[12], a31 = m[13], a32 = m[14], a33 = m[15];

    FLOAT b00 = a00 * a11 - a01 * a10;
    FLOAT b01 = a00 * a12 - a02 * a10;
    FLOAT b02 = a00 * a13 - a03 * a10;
    FLOAT b03 = a01 * a12 - a02 * a11;
    FLOAT b04 = a01 * a13 - a03 * a11;
    FLOAT b05 = a02 * a13 - a03 * a12;
    FLOAT b06 = a20 * a31 - a21 * a30;
    FLOAT b07 = a20 * a32 - a22 * a30;
    FLOAT b08 = a20 * a33 - a23 * a30;
    FLOAT b09 = a21 * a32 - a22 * a31;
    FLOAT b10 = a21 * a33 - a23 * a31;
    FLOAT b11 = a22 * a33 - a23 * a32;

    FLOAT det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;

    if (std::abs(det) < 1e-6f) {
        throw std::runtime_error("Matrix is singular and cannot be inverted");
    }

    FLOAT invDet = 1.0f / det;

    Matrix4 result;
    result.m[0] = (a11 * b11 - a12 * b10 + a13 * b09) * invDet;
    result.m[1] = (-a01 * b11 + a02 * b10 - a03 * b09) * invDet;
    result.m[2] = (a31 * b05 - a32 * b04 + a33 * b03) * invDet;
    result.m[3] = (-a21 * b05 + a22 * b04 - a23 * b03) * invDet;
    result.m[4] = (-a10 * b11 + a12 * b08 - a13 * b07) * invDet;
    result.m[5] = (a00 * b11 - a02 * b08 + a03 * b07) * invDet;
    result.m[6] = (-a30 * b05 + a32 * b02 - a33 * b01) * invDet;
    result.m[7] = (a20 * b05 - a22 * b02 + a23 * b01) * invDet;
    result.m[8] = (a10 * b10 - a11 * b08 + a13 * b06) * invDet;
    result.m[9] = (-a00 * b10 + a01 * b08 - a03 * b06) * invDet;
    result.m[10] = (a30 * b04 - a31 * b02 + a33 * b00) * invDet;
    result.m[11] = (-a20 * b04 + a21 * b02 - a23 * b00) * invDet;
    result.m[12] = (-a10 * b09 + a11 * b07 - a12 * b06) * invDet;
    result.m[13] = (a00 * b09 - a01 * b07 + a02 * b06) * invDet;
    result.m[14] = (-a30 * b03 + a31 * b01 - a32 * b00) * invDet;
    result.m[15] = (a20 * b03 - a21 * b01 + a22 * b00) * invDet;

    return result;
}

// 设置方法
void Matrix4::setIdentity() {
    m[0] = 1; m[1] = 0; m[2] = 0;  m[3] = 0;
    m[4] = 0; m[5] = 1; m[6] = 0;  m[7] = 0;
    m[8] = 0; m[9] = 0; m[10] = 1; m[11] = 0;
    m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
}

void Matrix4::setZero() {
    std::memset(m, 0, 16 * sizeof(FLOAT));
}

void Matrix4::set(const FLOAT* data) {
    std::memcpy(m, data, 16 * sizeof(FLOAT));
}

void Matrix4::set(const Matrix4& other) {
    std::memcpy(m, other.m, 16 * sizeof(FLOAT));
}

void Matrix4::getData(FLOAT* out) const {
    std::memcpy(out, m, 16 * sizeof(FLOAT));
}

// 比较
bool Matrix4::equals(const Matrix4& other, FLOAT epsilon) const {
    for (int i = 0; i < 16; i++) {
        if (std::abs(m[i] - other.m[i]) >= epsilon) {
            return false;
        }
    }
    return true;
}

// 静态工厂方法
Matrix4 Matrix4::identity() {
    return Matrix4();
}

Matrix4 Matrix4::zero() {
    return Matrix4(0.0f);
}

// 平移矩阵
Matrix4 Matrix4::translation(FLOAT x, FLOAT y, FLOAT z) {
    Matrix4 result;
    result.m[3] = x;
    result.m[7] = y;
    result.m[11] = z;
    return result;
}

Matrix4 Matrix4::translation(const Vector3& offset) {
    return translation(offset.x, offset.y, offset.z);
}

// 缩放矩阵
Matrix4 Matrix4::scale(FLOAT x, FLOAT y, FLOAT z) {
    Matrix4 result;
    result.m[0] = x;
    result.m[5] = y;
    result.m[10] = z;
    return result;
}

Matrix4 Matrix4::scale(const Vector3& scale) {
    return Matrix4::scale(scale.x, scale.y, scale.z);
}

Matrix4 Matrix4::scale(FLOAT uniform) {
    return Matrix4::scale(uniform, uniform, uniform);
}

// 旋转矩阵 - 绕X轴
Matrix4 Matrix4::rotationX(FLOAT angle) {
    Matrix4 result;
    FLOAT c = std::cos(angle);
    FLOAT s = std::sin(angle);
    
    result.m[5] = c;
    result.m[6] = -s;
    result.m[9] = s;
    result.m[10] = c;
    
    return result;
}

// 旋转矩阵 - 绕Y轴
Matrix4 Matrix4::rotationY(FLOAT angle) {
    Matrix4 result;
    FLOAT c = std::cos(angle);
    FLOAT s = std::sin(angle);
    
    result.m[0] = c;
    result.m[2] = s;
    result.m[8] = -s;
    result.m[10] = c;
    
    return result;
}

// 旋转矩阵 - 绕Z轴
Matrix4 Matrix4::rotationZ(FLOAT angle) {
    Matrix4 result;
    FLOAT c = std::cos(angle);
    FLOAT s = std::sin(angle);
    
    result.m[0] = c;
    result.m[1] = -s;
    result.m[4] = s;
    result.m[5] = c;
    
    return result;
}

// 旋转矩阵 - 绕任意轴
Matrix4 Matrix4::rotation(FLOAT angle, const Vector3& axis) {
    Vector3 a = axis.normalized();
    FLOAT c = std::cos(angle);
    FLOAT s = std::sin(angle);
    FLOAT t = 1.0f - c;
    
    Matrix4 result;
    result.m[0] = t * a.x * a.x + c;
    result.m[1] = t * a.x * a.y - s * a.z;
    result.m[2] = t * a.x * a.z + s * a.y;
    result.m[3] = 0;
    
    result.m[4] = t * a.x * a.y + s * a.z;
    result.m[5] = t * a.y * a.y + c;
    result.m[6] = t * a.y * a.z - s * a.x;
    result.m[7] = 0;
    
    result.m[8] = t * a.x * a.z - s * a.y;
    result.m[9] = t * a.y * a.z + s * a.x;
    result.m[10] = t * a.z * a.z + c;
    result.m[11] = 0;
    
    result.m[12] = 0;
    result.m[13] = 0;
    result.m[14] = 0;
    result.m[15] = 1;
    
    return result;
}

// LookAt视图矩阵
Matrix4 Matrix4::lookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
    Vector3 f = (target - eye).normalized();
    Vector3 s = f.cross(up).normalized();
    Vector3 u = s.cross(f);
    
    Matrix4 result;
    result.m[0] = s.x;
    result.m[1] = s.y;
    result.m[2] = s.z;
    result.m[3] = -s.dot(eye);
    
    result.m[4] = u.x;
    result.m[5] = u.y;
    result.m[6] = u.z;
    result.m[7] = -u.dot(eye);
    
    result.m[8] = -f.x;
    result.m[9] = -f.y;
    result.m[10] = -f.z;
    result.m[11] = f.dot(eye);
    
    result.m[12] = 0;
    result.m[13] = 0;
    result.m[14] = 0;
    result.m[15] = 1;
    
    return result;
}

// 透视投影矩阵
Matrix4 Matrix4::perspective(FLOAT fovY, FLOAT aspect, FLOAT near, FLOAT far) {
    FLOAT tanHalfFovy = std::tan(fovY / 2.0f);
    
    Matrix4 result(0.0f);
    result.m[0] = 1.0f / (aspect * tanHalfFovy);
    result.m[5] = 1.0f / tanHalfFovy;
    result.m[10] = -(far + near) / (far - near);
    result.m[11] = -(2.0f * far * near) / (far - near);
    result.m[14] = -1.0f;
    
    return result;
}

// 正交投影矩阵
Matrix4 Matrix4::orthographic(FLOAT left, FLOAT right, FLOAT bottom, FLOAT top, FLOAT near, FLOAT far) {
    Matrix4 result;
    result.m[0] = 2.0f / (right - left);
    result.m[3] = -(right + left) / (right - left);
    result.m[5] = 2.0f / (top - bottom);
    result.m[7] = -(top + bottom) / (top - bottom);
    result.m[10] = -2.0f / (far - near);
    result.m[11] = -(far + near) / (far - near);
    
    return result;
}

// 变换操作
void Matrix4::translate(FLOAT x, FLOAT y, FLOAT z) {
    *this = translation(x, y, z) * (*this);
}

void Matrix4::translate(const Vector3& offset) {
    translate(offset.x, offset.y, offset.z);
}

void Matrix4::rotate(FLOAT angle, const Vector3& axis) {
    *this = rotation(angle, axis) * (*this);
}

void Matrix4::scaleBy(FLOAT x, FLOAT y, FLOAT z) {
    *this = scale(x, y, z) * (*this);
}

void Matrix4::scaleBy(const Vector3& scale) {
    scaleBy(scale.x, scale.y, scale.z);
}

// 友元运算符
Matrix4 operator*(FLOAT scalar, const Matrix4& mat) {
    return mat * scalar;
}

RENDERER_NAMESPACE_END