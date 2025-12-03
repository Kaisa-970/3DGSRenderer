#pragma once

#include "Core/TypeDef.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API Vector3 {
public:
    FLOAT x;
    FLOAT y;
    FLOAT z;

    // 构造函数
    Vector3(FLOAT x = 0, FLOAT y = 0, FLOAT z = 0);
    ~Vector3();

    // Getter/Setter（可选，因为成员已公开）
    void setX(FLOAT x) { this->x = x; }
    void setY(FLOAT y) { this->y = y; }
    void setZ(FLOAT z) { this->z = z; }

    FLOAT getX() const { return x; }
    FLOAT getY() const { return y; }
    FLOAT getZ() const { return z; }

    // 基础数学操作
    FLOAT length() const;
    FLOAT lengthSquared() const;
    void normalize();
    Vector3 normalized() const;  // 返回归一化副本
    
    // 运算符重载（推荐使用，比 add/subtract 更直观）
    Vector3 operator+(const Vector3& other) const;
    Vector3 operator-(const Vector3& other) const;
    Vector3 operator*(FLOAT scalar) const;
    Vector3 operator/(FLOAT scalar) const;
    Vector3 operator-() const;  // 取负：-vec
    
    Vector3& operator+=(const Vector3& other);
    Vector3& operator-=(const Vector3& other);
    Vector3& operator*=(FLOAT scalar);
    Vector3& operator/=(FLOAT scalar);
    
    bool operator==(const Vector3& other) const;
    bool operator!=(const Vector3& other) const;
    
    // 数组访问：vec[0], vec[1], vec[2]
    FLOAT& operator[](int index);
    const FLOAT& operator[](int index) const;
    
    // 点乘和叉乘
    FLOAT dot(const Vector3& other) const;
    Vector3 cross(const Vector3& other) const;
    
    // 保留旧接口（兼容性）
    void scale(FLOAT scalar);
    void add(const Vector3& other);
    void subtract(const Vector3& other);
    void multiply(const Vector3& other);
    void divide(const Vector3& other);
    void negate();
    
    // 实用方法
    void zero();
    void set(FLOAT x, FLOAT y, FLOAT z);
    void set(const Vector3& other);
    bool equals(const Vector3& other, FLOAT epsilon = 1e-6f) const;
    
    // 静态工具方法
    static FLOAT distance(const Vector3& a, const Vector3& b);
    static Vector3 lerp(const Vector3& a, const Vector3& b, FLOAT t);
};

// 友元运算符：支持 2.0f * vec（scalar 在左边）
RENDERER_API Vector3 operator*(FLOAT scalar, const Vector3& vec);


class RENDERER_API Vector2 {
public:
    FLOAT x;
    FLOAT y;

    // 构造函数
    Vector2(FLOAT x = 0, FLOAT y = 0);
    ~Vector2();

    Vector2 operator+(const Vector2& other) const;
    Vector2 operator-(const Vector2& other) const;
    Vector2 operator*(FLOAT scalar) const;
    Vector2 operator/(FLOAT scalar) const;
    Vector2 operator-() const;  // 取负：-vec
        
    FLOAT& operator[](int index);
    const FLOAT& operator[](int index) const;
    
    FLOAT dot(const Vector2& other) const;
    static Vector2 lerp(const Vector2& a, const Vector2& b, FLOAT t);
};
    
// 友元运算符：支持 2.0f * vec（scalar 在左边）
RENDERER_API Vector2 operator*(FLOAT scalar, const Vector2& vec);
    
RENDERER_NAMESPACE_END