#pragma once

#include "Core/TypeDef.h"
#include "Vector.h"
#include "Matrix.h"
#include <cmath>

RENDERER_NAMESPACE_BEGIN

// 四元数类（用于表示旋转）
class RENDERER_API Quaternion {
public:
    FLOAT x, y, z, w;  // w是实部

    // 构造函数
    Quaternion() : x(0), y(0), z(0), w(1) {}
    Quaternion(FLOAT x, FLOAT y, FLOAT z, FLOAT w) : x(x), y(y), z(z), w(w) {}
    Quaternion(const FLOAT* data) : x(data[0]), y(data[1]), z(data[2]), w(data[3]) {}

    // 归一化
    Quaternion normalized() const {
        FLOAT len = std::sqrt(x*x + y*y + z*z + w*w);
        if (len < 1e-8f) return Quaternion(0, 0, 0, 1);
        return Quaternion(x/len, y/len, z/len, w/len);
    }

    // 转换为旋转矩阵（3x3）
    void toRotationMatrix(FLOAT* mat3x3) const {
        FLOAT xx = x * x, yy = y * y, zz = z * z;
        FLOAT xy = x * y, xz = x * z, yz = y * z;
        FLOAT wx = w * x, wy = w * y, wz = w * z;

        // 行主序 3x3 矩阵
        mat3x3[0] = 1 - 2*(yy + zz);
        mat3x3[1] = 2*(xy - wz);
        mat3x3[2] = 2*(xz + wy);
        
        mat3x3[3] = 2*(xy + wz);
        mat3x3[4] = 1 - 2*(xx + zz);
        mat3x3[5] = 2*(yz - wx);
        
        mat3x3[6] = 2*(xz - wy);
        mat3x3[7] = 2*(yz + wx);
        mat3x3[8] = 1 - 2*(xx + yy);
    }

    // 转换为4x4矩阵
    Matrix4 toMatrix4() const {
        FLOAT mat3x3[9];
        toRotationMatrix(mat3x3);
        
        return Matrix4(
            mat3x3[0], mat3x3[1], mat3x3[2], 0,
            mat3x3[3], mat3x3[4], mat3x3[5], 0,
            mat3x3[6], mat3x3[7], mat3x3[8], 0,
            0, 0, 0, 1
        );
    }

    // 四元数乘法
    Quaternion operator*(const Quaternion& q) const {
        return Quaternion(
            w*q.x + x*q.w + y*q.z - z*q.y,
            w*q.y - x*q.z + y*q.w + z*q.x,
            w*q.z + x*q.y - y*q.x + z*q.w,
            w*q.w - x*q.x - y*q.y - z*q.z
        );
    }

    // 共轭
    Quaternion conjugate() const {
        return Quaternion(-x, -y, -z, w);
    }

    // 从轴角创建
    static Quaternion fromAxisAngle(const Vector3& axis, FLOAT angle) {
        FLOAT halfAngle = angle * 0.5f;
        FLOAT s = std::sin(halfAngle);
        return Quaternion(
            axis.x * s,
            axis.y * s,
            axis.z * s,
            std::cos(halfAngle)
        );
    }
};

RENDERER_NAMESPACE_END

