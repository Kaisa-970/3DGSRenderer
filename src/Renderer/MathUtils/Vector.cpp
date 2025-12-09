#include "Vector.h"
#include <cmath>
#include <stdexcept>

RENDERER_NAMESPACE_BEGIN

const Vector3 Vector3::ZERO(0.0f, 0.0f, 0.0f);
const Vector3 Vector3::ONE(1.0f, 1.0f, 1.0f);

// 构造函数
Vector3::Vector3(FLOAT x, FLOAT y, FLOAT z)
    : x(x), y(y), z(z) {
}

Vector3::~Vector3() {
}

// 长度计算
FLOAT Vector3::length() const {
    return std::sqrt(x * x + y * y + z * z);
}

FLOAT Vector3::lengthSquared() const {
    return x * x + y * y + z * z;
}

// 归一化
void Vector3::normalize() {
    FLOAT len = length();
    if (len > 1e-6f) {
        x /= len;
        y /= len;
        z /= len;
    }
}

Vector3 Vector3::normalized() const {
    Vector3 result(*this);
    result.normalize();
    return result;
}

// 运算符重载 - 加法
Vector3 Vector3::operator+(const Vector3& other) const {
    return Vector3(x + other.x, y + other.y, z + other.z);
}

Vector3 Vector3::operator-(const Vector3& other) const {
    return Vector3(x - other.x, y - other.y, z - other.z);
}

Vector3 Vector3::operator*(FLOAT scalar) const {
    return Vector3(x * scalar, y * scalar, z * scalar);
}

Vector3 Vector3::operator/(FLOAT scalar) const {
    if (std::abs(scalar) < 1e-6f) {
        throw std::runtime_error("Division by zero in Vector3");
    }
    return Vector3(x / scalar, y / scalar, z / scalar);
}

Vector3 Vector3::operator-() const {
    return Vector3(-x, -y, -z);
}

// 复合赋值运算符
Vector3& Vector3::operator+=(const Vector3& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

Vector3& Vector3::operator-=(const Vector3& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

Vector3& Vector3::operator*=(FLOAT scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

Vector3& Vector3::operator/=(FLOAT scalar) {
    if (std::abs(scalar) < 1e-6f) {
        throw std::runtime_error("Division by zero in Vector3");
    }
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
}

// 比较运算符
bool Vector3::operator==(const Vector3& other) const {
    return equals(other, 1e-6f);
}

bool Vector3::operator!=(const Vector3& other) const {
    return !equals(other, 1e-6f);
}

// 数组访问
FLOAT& Vector3::operator[](int index) {
    switch (index) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        default: throw std::out_of_range("Vector3 index out of range");
    }
}

const FLOAT& Vector3::operator[](int index) const {
    switch (index) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        default: throw std::out_of_range("Vector3 index out of range");
    }
}

// 点乘
FLOAT Vector3::dot(const Vector3& other) const {
    return x * other.x + y * other.y + z * other.z;
}

// 叉乘
Vector3 Vector3::cross(const Vector3& other) const {
    return Vector3(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    );
}

// 旧接口（兼容性）
void Vector3::scale(FLOAT scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
}

void Vector3::add(const Vector3& other) {
    x += other.x;
    y += other.y;
    z += other.z;
}

void Vector3::subtract(const Vector3& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
}

void Vector3::multiply(const Vector3& other) {
    x *= other.x;
    y *= other.y;
    z *= other.z;
}

void Vector3::divide(const Vector3& other) {
    if (std::abs(other.x) < 1e-6f || std::abs(other.y) < 1e-6f || std::abs(other.z) < 1e-6f) {
        throw std::runtime_error("Division by zero in Vector3");
    }
    x /= other.x;
    y /= other.y;
    z /= other.z;
}

void Vector3::negate() {
    x = -x;
    y = -y;
    z = -z;
}

// 实用方法
void Vector3::zero() {
    x = 0;
    y = 0;
    z = 0;
}

void Vector3::set(FLOAT x, FLOAT y, FLOAT z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

void Vector3::set(const Vector3& other) {
    x = other.x;
    y = other.y;
    z = other.z;
}

bool Vector3::equals(const Vector3& other, FLOAT epsilon) const {
    return std::abs(x - other.x) < epsilon &&
           std::abs(y - other.y) < epsilon &&
           std::abs(z - other.z) < epsilon;
}

// 静态工具方法
FLOAT Vector3::distance(const Vector3& a, const Vector3& b) {
    FLOAT dx = a.x - b.x;
    FLOAT dy = a.y - b.y;
    FLOAT dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

Vector3 Vector3::lerp(const Vector3& a, const Vector3& b, FLOAT t) {
    // 限制 t 在 [0, 1] 范围内
    t = std::max(0.0f, std::min(1.0f, t));
    return Vector3(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    );
}

// 友元运算符实现
Vector3 operator*(FLOAT scalar, const Vector3& vec) {
    return vec * scalar;
}

// 构造函数
Vector2::Vector2(FLOAT x, FLOAT y)
    : x(x), y(y) {
}

Vector2::~Vector2() {
}

Vector2 Vector2::operator+(const Vector2& other) const {
    return Vector2(x + other.x, y + other.y);
}

Vector2 Vector2::operator-(const Vector2& other) const {
    return Vector2(x - other.x, y - other.y);
}

Vector2 Vector2::operator*(FLOAT scalar) const {
    return Vector2(x * scalar, y * scalar);
}

Vector2 Vector2::operator/(FLOAT scalar) const {
    if (std::abs(scalar) < 1e-6f) {
        throw std::runtime_error("Division by zero in Vector2");
    }
    return Vector2(x / scalar, y / scalar);
}

Vector2 Vector2::operator-() const {
    return Vector2(-x, -y);
}

FLOAT& Vector2::operator[](int index) {
    switch (index) {
        case 0: return x;
        case 1: return y;
        default: throw std::out_of_range("Vector2 index out of range");
    }
}

const FLOAT& Vector2::operator[](int index) const {
    switch (index) {
        case 0: return x;
        case 1: return y;
        default: throw std::out_of_range("Vector2 index out of range");
    }
}

// 友元运算符实现
Vector2 operator*(FLOAT scalar, const Vector2& vec) {
    return vec * scalar;
}

RENDERER_NAMESPACE_END

