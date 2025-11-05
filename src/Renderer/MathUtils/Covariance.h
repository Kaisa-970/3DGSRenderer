#pragma once

#include "Core/TypeDef.h"
#include "Quaternion.h"
#include "Matrix.h"
#include <cmath>

RENDERER_NAMESPACE_BEGIN

// 协方差矩阵计算工具
class RENDERER_API CovarianceUtils {
public:
    // 从缩放和旋转四元数计算3D协方差矩阵
    // 返回3x3矩阵（行主序存储在9个元素中）
    static void compute3DCovariance(
        const FLOAT* scale,      // [sx, sy, sz]
        const FLOAT* rotation,   // [qx, qy, qz, qw]
        FLOAT* cov3D)            // 输出：3x3协方差矩阵
    {
        // 1. 构建旋转矩阵 R
        Quaternion q(rotation);
        q = q.normalized();
        FLOAT R[9];
        q.toRotationMatrix(R);

        // 2. 构建缩放矩阵 S
        FLOAT S[9] = {
            scale[0], 0, 0,
            0, scale[1], 0,
            0, 0, scale[2]
        };

        // 3. 计算 M = R * S
        FLOAT M[9];
        matmul3x3(R, S, M);

        // 4. 计算协方差 Σ = M * M^T
        FLOAT MT[9];
        transpose3x3(M, MT);
        matmul3x3(M, MT, cov3D);
    }

    // 将3D协方差投影到2D屏幕空间
    // 这是简化版本，完整版需要雅可比矩阵
    static void project3DCovariance(
        const FLOAT* cov3D,           // 3x3输入协方差
        const FLOAT* viewMatrix,      // 4x4视图矩阵
        const FLOAT* /* projMatrix */,// 4x4投影矩阵（未使用）
        const FLOAT* position,        // 3D位置
        FLOAT focal_x,                // 焦距x
        FLOAT focal_y,                // 焦距y
        FLOAT* cov2D)                 // 输出：2x2协方差矩阵
    {
        // 计算视图空间位置
        FLOAT viewPos[4] = {position[0], position[1], position[2], 1.0f};
        FLOAT transformed[4];
        matmul4x4_vec4(viewMatrix, viewPos, transformed);

        FLOAT t_x = transformed[0];
        FLOAT t_y = transformed[1];
        FLOAT t_z = transformed[2];

        // 限制深度避免除零
        FLOAT limz = t_z > 0.001f ? t_z : 0.001f;
        FLOAT limz2 = limz * limz;

        // 提取视图矩阵的旋转部分（3x3）
        FLOAT W[9] = {
            viewMatrix[0], viewMatrix[1], viewMatrix[2],
            viewMatrix[4], viewMatrix[5], viewMatrix[6],
            viewMatrix[8], viewMatrix[9], viewMatrix[10]
        };

        // 计算 T = W * Σ
        FLOAT T[9];
        matmul3x3(W, cov3D, T);

        // 计算 Σ' = T * W^T
        FLOAT WT[9];
        transpose3x3(W, WT);
        FLOAT cov3D_view[9];
        matmul3x3(T, WT, cov3D_view);

        // 雅可比矩阵：考虑深度变化的修正项（3x2矩阵，列主序）
        // J = [fx/z,  0  ]
        //     [ 0,  fy/z ]
        //     [-fx*x/z², -fy*y/z²]
        FLOAT J_full[6] = {
            focal_x / limz, 0, -(focal_x * t_x) / limz2,
            0, focal_y / limz, -(focal_y * t_y) / limz2
        };

        // 计算 2D 协方差：J^T * Σ'[3x3] * J
        cov2D[0] = J_full[0] * (cov3D_view[0] * J_full[0] + cov3D_view[1] * J_full[1] + cov3D_view[2] * J_full[2]) +
                   J_full[1] * (cov3D_view[3] * J_full[0] + cov3D_view[4] * J_full[1] + cov3D_view[5] * J_full[2]) +
                   J_full[2] * (cov3D_view[6] * J_full[0] + cov3D_view[7] * J_full[1] + cov3D_view[8] * J_full[2]);

        cov2D[1] = J_full[0] * (cov3D_view[0] * J_full[3] + cov3D_view[1] * J_full[4] + cov3D_view[2] * J_full[5]) +
                   J_full[1] * (cov3D_view[3] * J_full[3] + cov3D_view[4] * J_full[4] + cov3D_view[5] * J_full[5]) +
                   J_full[2] * (cov3D_view[6] * J_full[3] + cov3D_view[7] * J_full[4] + cov3D_view[8] * J_full[5]);

        cov2D[2] = J_full[3] * (cov3D_view[0] * J_full[0] + cov3D_view[1] * J_full[1] + cov3D_view[2] * J_full[2]) +
                   J_full[4] * (cov3D_view[3] * J_full[0] + cov3D_view[4] * J_full[1] + cov3D_view[5] * J_full[2]) +
                   J_full[5] * (cov3D_view[6] * J_full[0] + cov3D_view[7] * J_full[1] + cov3D_view[8] * J_full[2]);

        cov2D[3] = J_full[3] * (cov3D_view[0] * J_full[3] + cov3D_view[1] * J_full[4] + cov3D_view[2] * J_full[5]) +
                   J_full[4] * (cov3D_view[3] * J_full[3] + cov3D_view[4] * J_full[4] + cov3D_view[5] * J_full[5]) +
                   J_full[5] * (cov3D_view[6] * J_full[3] + cov3D_view[7] * J_full[4] + cov3D_view[8] * J_full[5]);

        // 添加低通滤波器防止走样
        cov2D[0] += 0.3f;
        cov2D[3] += 0.3f;
    }

    // 计算2x2矩阵的逆
    static bool inverse2x2(const FLOAT* mat, FLOAT* inv) {
        FLOAT det = mat[0] * mat[3] - mat[1] * mat[2];
        if (std::abs(det) < 1e-10f) return false;

        FLOAT invDet = 1.0f / det;
        inv[0] = mat[3] * invDet;
        inv[1] = -mat[1] * invDet;
        inv[2] = -mat[2] * invDet;
        inv[3] = mat[0] * invDet;
        return true;
    }

    // 计算2x2矩阵的特征值（用于确定椭圆大小）
    static void eigenvalues2x2(const FLOAT* mat, FLOAT& lambda1, FLOAT& lambda2) {
        FLOAT trace = mat[0] + mat[3];
        FLOAT det = mat[0] * mat[3] - mat[1] * mat[2];
        FLOAT discriminant = trace * trace - 4 * det;
        if (discriminant < 0) discriminant = 0;
        FLOAT sqrtDisc = std::sqrt(discriminant);
        lambda1 = (trace + sqrtDisc) * 0.5f;
        lambda2 = (trace - sqrtDisc) * 0.5f;
    }

private:
    // 3x3矩阵乘法 C = A * B
    static void matmul3x3(const FLOAT* A, const FLOAT* B, FLOAT* C) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                C[i*3+j] = 0;
                for (int k = 0; k < 3; k++) {
                    C[i*3+j] += A[i*3+k] * B[k*3+j];
                }
            }
        }
    }

    // 3x3矩阵转置
    static void transpose3x3(const FLOAT* A, FLOAT* AT) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                AT[j*3+i] = A[i*3+j];
            }
        }
    }

    // 4x4矩阵乘向量
    static void matmul4x4_vec4(const FLOAT* M, const FLOAT* v, FLOAT* result) {
        for (int i = 0; i < 4; i++) {
            result[i] = 0;
            for (int j = 0; j < 4; j++) {
                result[i] += M[i*4+j] * v[j];
            }
        }
    }
};

RENDERER_NAMESPACE_END

