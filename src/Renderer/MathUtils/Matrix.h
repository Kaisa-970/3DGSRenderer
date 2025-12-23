#pragma once

#include "Core/BaseDef.h"
#include "Vector.h"

RENDERER_NAMESPACE_BEGIN

// 4x4 矩阵类 (行主序存储)
class RENDERER_API Matrix4
{
public:
    // 数据存储：16个元素，行主序
    FLOAT m[16];

    // 构造函数
    Matrix4();                  // 默认构造为单位矩阵
    Matrix4(FLOAT diagonal);    // 对角矩阵
    Matrix4(const FLOAT *data); // 从数组构造
    Matrix4(FLOAT m00, FLOAT m01, FLOAT m02, FLOAT m03, FLOAT m10, FLOAT m11, FLOAT m12, FLOAT m13, FLOAT m20,
            FLOAT m21, FLOAT m22, FLOAT m23, FLOAT m30, FLOAT m31, FLOAT m32, FLOAT m33);
    ~Matrix4();

    // 访问元素
    FLOAT &operator()(int row, int col);
    const FLOAT &operator()(int row, int col) const;
    FLOAT &operator[](int index);
    const FLOAT &operator[](int index) const;

    // 矩阵运算
    Matrix4 operator+(const Matrix4 &other) const;
    Matrix4 operator-(const Matrix4 &other) const;
    Matrix4 operator*(const Matrix4 &other) const;
    Matrix4 operator*(FLOAT scalar) const;
    Vector3 operator*(const Vector3 &vec) const; // 矩阵 * 向量 (假设w=1)

    Matrix4 &operator+=(const Matrix4 &other);
    Matrix4 &operator-=(const Matrix4 &other);
    Matrix4 &operator*=(const Matrix4 &other);
    Matrix4 &operator*=(FLOAT scalar);

    bool operator==(const Matrix4 &other) const;
    bool operator!=(const Matrix4 &other) const;

    // 矩阵操作
    Matrix4 transpose() const;
    Matrix4 inverse() const;
    FLOAT determinant() const;

    // 设置方法
    void setIdentity();
    void setZero();
    void set(const FLOAT *data);
    void set(const Matrix4 &other);

    // 获取数据
    const FLOAT *data() const
    {
        return m;
    }
    FLOAT *data()
    {
        return m;
    }
    void getData(FLOAT *out) const;

    // 实用方法
    bool equals(const Matrix4 &other, FLOAT epsilon = 1e-6f) const;

    // 静态工厂方法 - 创建特殊矩阵
    static Matrix4 identity();
    static Matrix4 zero();

    // 变换矩阵
    static Matrix4 translation(FLOAT x, FLOAT y, FLOAT z);
    static Matrix4 translation(const Vector3 &offset);
    static Matrix4 scale(FLOAT x, FLOAT y, FLOAT z);
    static Matrix4 scale(const Vector3 &scale);
    static Matrix4 scale(FLOAT uniform);

    // 旋转矩阵（角度使用弧度）
    static Matrix4 rotationX(FLOAT angle);
    static Matrix4 rotationY(FLOAT angle);
    static Matrix4 rotationZ(FLOAT angle);
    static Matrix4 rotation(FLOAT angle, const Vector3 &axis); // 绕任意轴旋转

    // 视图矩阵
    static Matrix4 lookAt(const Vector3 &eye, const Vector3 &target, const Vector3 &up);

    // 投影矩阵
    static Matrix4 perspective(FLOAT fovY, FLOAT aspect, FLOAT near, FLOAT far);
    static Matrix4 orthographic(FLOAT left, FLOAT right, FLOAT bottom, FLOAT top, FLOAT near, FLOAT far);

    // 变换操作（修改当前矩阵）
    void translate(FLOAT x, FLOAT y, FLOAT z);
    void translate(const Vector3 &offset);
    void rotate(FLOAT angle, const Vector3 &axis);
    void scaleBy(FLOAT x, FLOAT y, FLOAT z);
    void scaleBy(const Vector3 &scale);
};

// 友元运算符
RENDERER_API Matrix4 operator*(FLOAT scalar, const Matrix4 &mat);

RENDERER_NAMESPACE_END
